#include "DeviceViewModel.h"
#include "model/Settings.h"
#include "model/Logger.h"
#include "platform/SelfServiceClient.h"
#include "platform/NetworkAdapter.h"

#include <QMetaObject>
#include <QMetaType>
#include <QVariantMap>
#include <QDebug>

// ----------------------------------------------------------------------------
// DeviceWorker：在独立线程中执行阻塞式网络请求，结果跨线程回调回 UI 线程
// ----------------------------------------------------------------------------

void DeviceWorker::refresh()
{
    QString err;
    const QVariantList devs = Platform::SelfServiceClient::fetchDevices(
        m_settings->username(), m_settings->password(), &err);
    // 回到 UI 线程更新属性
    QMetaObject::invokeMethod(m_vm, "onFetchResult", Qt::QueuedConnection,
                              Q_ARG(QVariantList, devs), Q_ARG(QString, err));
}

void DeviceWorker::kick(const QString &sessionId)
{
    QString err;
    const bool ok = Platform::SelfServiceClient::kickDevice(
        m_settings->username(), m_settings->password(), sessionId, &err);
    QMetaObject::invokeMethod(m_vm, "onKickResult", Qt::QueuedConnection,
                              Q_ARG(bool, ok), Q_ARG(QString, err),
                              Q_ARG(QString, sessionId));
}

void DeviceWorker::fetchCountOnly()
{
    QString err;
    const QVariantList devs = Platform::SelfServiceClient::fetchDevices(
        m_settings->username(), m_settings->password(), &err);
    // 回到 UI 线程播报数量（不改 devices/loading）
    QMetaObject::invokeMethod(m_vm, "onCountFetched", Qt::QueuedConnection,
                              Q_ARG(int, devs.size()), Q_ARG(QString, err));
}

void DeviceWorker::kickAllExceptSelf()
{
    QString err;
    const QVariantList devs = Platform::SelfServiceClient::fetchDevices(
        m_settings->username(), m_settings->password(), &err);

    int total = 0, success = 0, failed = 0;
    if (err.isEmpty()) {
        const QString selfMac =
            DeviceViewModel::normalizeMac(Platform::NetworkAdapter::getMacAddress());
        for (const QVariant &vd : devs) {
            const QVariantMap m = vd.toMap();
            const QString devMac =
                DeviceViewModel::normalizeMac(m.value(QStringLiteral("mac")).toString());
            if (devMac == selfMac)
                continue;                       // 不踢本机
            const QString sid = m.value(QStringLiteral("sessionId")).toString();
            if (sid.isEmpty())
                continue;
            QString kerr;
            const bool ok = Platform::SelfServiceClient::kickDevice(
                m_settings->username(), m_settings->password(), sid, &kerr);
            ++total;
            if (ok) ++success; else ++failed;
        }
    }
    QMetaObject::invokeMethod(m_vm, "onKickAllResult", Qt::QueuedConnection,
                              Q_ARG(int, total), Q_ARG(int, success),
                              Q_ARG(int, failed), Q_ARG(QString, err));
}

// ----------------------------------------------------------------------------
// DeviceViewModel
// ----------------------------------------------------------------------------

DeviceViewModel::DeviceViewModel(Settings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    // QVariantList 跨线程 QueuedConnection 需要已在元对象系统中注册
    qRegisterMetaType<QVariantList>("QVariantList");

    const QString raw = m_settings ? m_settings->username() : QString();
    m_accountMasked = maskAccount(raw);
    emit accountChanged();

    // 自建独立线程（独立于 App 的认证 worker，互不干扰）
    m_thread = new QThread();
    m_worker = new DeviceWorker(m_settings);
    m_worker->setViewModel(this);
    m_worker->moveToThread(m_thread);
    m_thread->start();
}

DeviceViewModel::~DeviceViewModel()
{
    // worker 在子线程内运行阻塞式网络调用（自带嵌套 QEventLoop），
    // 因此不能在其调用进行中用 deleteLater（会被嵌套循环立即处理 → 对象正被执行时析构）。
    // 正确做法：退出线程并轮询等待其真正结束，再移回 UI 线程安全删除 worker。
    if (m_thread) {
        m_thread->quit();
        for (int i = 0; i < 40 && !m_thread->isFinished(); ++i)
            m_thread->wait(1000);   // 最坏约 21s；大量设备踢除可能更久，超时后 detach

        if (m_thread->isFinished()) {
            if (m_worker) {
                m_worker->moveToThread(this->thread());
                delete m_worker;
                m_worker = nullptr;
            }
            delete m_thread;
        } else {
            // 极端情况（在途操作仍未结束）：放弃同步等待，detach 交由进程退出回收，避免崩溃。
            if (m_worker) { m_worker->moveToThread(nullptr); m_worker = nullptr; }
            m_thread->moveToThread(nullptr);
        }
        m_thread = nullptr;
    }
}

void DeviceViewModel::refresh()
{
    if (m_loading) return;            // 防抖：上一次未完成则忽略
    setLoading(true);
    setErrorMessage(QString());
    QMetaObject::invokeMethod(m_worker, "refresh", Qt::QueuedConnection);
}

void DeviceViewModel::kick(const QString &sessionId)
{
    if (m_loading) return;
    setLoading(true);
    setErrorMessage(QString());
    QMetaObject::invokeMethod(m_worker, "kick", Qt::QueuedConnection,
                              Q_ARG(QString, sessionId));
}

void DeviceViewModel::kickAllExceptSelf()
{
    if (m_loading) return;
    setLoading(true);
    setErrorMessage(QString());
    QMetaObject::invokeMethod(m_worker, "kickAllExceptSelf", Qt::QueuedConnection);
}

void DeviceViewModel::fetchCountOnly()
{
    // 在 UI 线程执行：防抖 + 并发保护，绝不在 UI 线程做网络请求
    if (m_countFetching)
        return;                       // 上一次还在途中，直接忽略，避免堆叠多个请求
    if (m_lastCountFetch.isValid() && m_lastCountFetch.elapsed() < 60000)
        return;                       // 60s 最小间隔，避免对自助服务无谓施压
    m_countFetching = true;
    m_lastCountFetch.restart();
    QMetaObject::invokeMethod(m_worker, "fetchCountOnly", Qt::QueuedConnection);
}

void DeviceViewModel::onCountFetched(int count, const QString &error)
{
    m_countFetching = false;
    if (!error.isEmpty()) {
        qWarning() << "[Device] 静默拉取设备数失败:" << error;
        emit deviceCountFetchFailed(error);   // 安静降级：失败时不弹设备数
        return;
    }
    emit deviceCountFetched(count);
}

void DeviceViewModel::onFetchResult(const QVariantList &devices, const QString &error)
{
    setLoading(false);
    if (!error.isEmpty()) {
        qWarning() << "[Device] 拉取在线设备失败:" << error;
        setErrorMessage(error);       // 保留旧列表，仅提示错误
        return;
    }
    qInfo() << "[Device] 在线设备已获取，共" << devices.size() << "台";

    const QString selfMac = normalizeMac(Platform::NetworkAdapter::getMacAddress());
    QVariantList processed;
    processed.reserve(devices.size());
    for (const QVariant &vd : devices) {
        QVariantMap m = vd.toMap();
        const QString devMac = normalizeMac(m.value(QStringLiteral("mac")).toString());
        m[QStringLiteral("isSelf")] = (devMac == selfMac);
        processed.append(m);
    }

    setErrorMessage(QString());       // 成功清空错误
    setDevices(processed);
}

void DeviceViewModel::onKickResult(bool ok, const QString &error, const QString &sessionId)
{
    Q_UNUSED(sessionId)
    setLoading(false);
    if (!ok) {
        setErrorMessage(error.isEmpty() ? QStringLiteral("踢除设备失败") : error);
    } else {
        setErrorMessage(QString());
        refresh();                     // 成功后刷新列表
    }
}

void DeviceViewModel::onKickAllResult(int total, int success, int failed, const QString &error)
{
    setLoading(false);
    if (!error.isEmpty()) {
        setErrorMessage(error);
    } else if (failed > 0) {
        setErrorMessage(QStringLiteral("批量踢除：成功 %1，失败 %2").arg(success).arg(failed));
    } else {
        setErrorMessage(QString());
    }

    // 统计日志（走项目 Logger，未接入则退回 qInfo）
    const QString msg = QStringLiteral("[设备管理] 批量踢除完成: 共%1 成功%2 失败%3")
                           .arg(total).arg(success).arg(failed);
    if (m_logger)
        m_logger->log(msg);
    else
        qInfo() << msg;

    refresh();                         // 完成后刷新列表
}

void DeviceViewModel::setLoading(bool v)
{
    if (m_loading == v) return;
    m_loading = v;
    emit loadingChanged();
}

void DeviceViewModel::setErrorMessage(const QString &msg)
{
    if (m_errorMessage == msg) return;
    m_errorMessage = msg;
    emit errorMessageChanged();
}

void DeviceViewModel::setDevices(const QVariantList &devs)
{
    m_devices = devs;
    emit devicesChanged();
}

QString DeviceViewModel::maskAccount(const QString &account)
{
    if (account.isEmpty()) return QString();
    const int len = account.length();
    if (len <= 2) return account;
    if (len <= 7) {
        // 较短：保留首尾，中间星号
        return account.left(1)
               + QString(len - 2, QLatin1Char('*'))
               + account.right(1);
    }
    // 常规：保留前 3 后 4
    return account.left(3) + QStringLiteral("****") + account.right(4);
}

QString DeviceViewModel::normalizeMac(const QString &mac)
{
    QString out;
    out.reserve(mac.length());
    for (const QChar &c : mac) {
        if (c == QLatin1Char(':') || c == QLatin1Char('-'))
            continue;
        out.append(c.toUpper());
    }
    return out;
}
