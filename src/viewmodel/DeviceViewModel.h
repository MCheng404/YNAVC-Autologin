#pragma once

#include <QObject>
#include <QVariantList>
#include <QThread>
#include <QElapsedTimer>
#include <QString>

class Settings;
class Logger;
class DeviceViewModel;

/**
 * @brief DeviceViewModel 的内部 worker：执行网络请求，存活于独立线程。
 *
 * 必须定义在 DeviceViewModel 的**外部** —— moc 不支持嵌套类的元对象
 * （会报 "Meta object features not supported for nested classes"），
 * 而它需要 Q_OBJECT 才能通过 QueuedConnection 接收跨线程调用。
 */
class DeviceWorker : public QObject {
    Q_OBJECT
public:
    explicit DeviceWorker(Settings *settings, QObject *parent = nullptr)
        : QObject(parent), m_settings(settings) {}
    void setViewModel(DeviceViewModel *vm) { m_vm = vm; }

public slots:
    void refresh();
    void kick(const QString &sessionId);
    void kickAllExceptSelf();
    void fetchCountOnly();              // 静默拉取一次在线设备数（不改动 devices/loading）

private:
    Settings        *m_settings = nullptr;
    DeviceViewModel *m_vm       = nullptr;
};

/**
 * @brief 在线设备管理 ViewModel（暴露给 QML 的 deviceVM）
 *
 * 网络操作全部在内部自建的 QThread（m_thread）中执行，绝不阻塞 UI 线程。
 * 调用 SelfServiceClient 的静态方法走「登录 → 操作 → 回 UI 线程更新属性」流程。
 *
 * 属性：
 *  - devices      : QVariantList，每项含 sessionId/ip/mac/terminalType/loginTime/useTime/isSelf
 *  - loading      : 是否正在请求
 *  - errorMessage : 错误提示（成功时清空）
 *  - account      : 脱敏后的账号（如 132****8719）
 */
class DeviceViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString account READ account NOTIFY accountChanged)

public:
    explicit DeviceViewModel(Settings *settings, QObject *parent = nullptr);
    ~DeviceViewModel();

    QVariantList devices() const { return m_devices; }
    bool loading() const { return m_loading; }
    QString errorMessage() const { return m_errorMessage; }
    QString account() const { return m_accountMasked; }

    /** 可选：接入项目日志器，用于记录批量踢除的统计结果（不设置则退回 qInfo） */
    void setLogger(Logger *logger) { m_logger = logger; }

    Q_INVOKABLE void refresh();                 // 拉取设备列表
    /** 自动触发的刷新（如"打开设置时静默获取一次"）。
     *  与 refresh() 的区别：带最小间隔节流 —— 短时间内反复打开设置时
     *  不会重复登录自助服务（每次 0.3~1s，对服务端也是无谓压力）。
     *  手动点「刷新」按钮仍走 refresh()，不受节流限制。 */
    Q_INVOKABLE void refreshIfStale(int minIntervalSec = 10);
    Q_INVOKABLE void kick(const QString &sessionId);      // 踢单个
    Q_INVOKABLE void kickAllExceptSelf();                 // 踢掉除本机 MAC 之外的全部
    Q_INVOKABLE void fetchCountOnly();        // 静默拉取一次设备数，结果通过信号播报

    // 工具函数。放在 public：DeviceWorker 定义在类外，需要调用它们。
    static QString maskAccount(const QString &account);
    static QString normalizeMac(const QString &mac);

signals:
    void devicesChanged();
    void loadingChanged();
    void errorMessageChanged();
    void accountChanged();
    void deviceCountFetched(int count);        // 静默拉取成功：当前在线设备数
    void deviceCountFetchFailed(QString err);  // 静默拉取失败：安静降级，不弹设备数

private slots:
    // 由 worker 线程通过 QueuedConnection 回调，运行在 UI 线程
    void onFetchResult(const QVariantList &devices, const QString &error);
    void onKickResult(bool ok, const QString &error, const QString &sessionId);
    void onKickAllResult(int total, int success, int failed, const QString &error);
    void onCountFetched(int count, const QString &error);  // 静默拉取的回调

private:
    void setLoading(bool v);
    void setErrorMessage(const QString &msg);
    void setDevices(const QVariantList &devs);

    Settings *m_settings     = nullptr;
    Logger   *m_logger       = nullptr;

    QVariantList m_devices;
    bool         m_loading    = false;
    QString      m_errorMessage;
    QString      m_accountMasked;

    QThread *m_thread = nullptr;

    // 静默设备数拉取的防抖/并发保护（均在 UI 线程访问）
    bool          m_countFetching = false;   // 上一次未完成则忽略，避免堆叠请求
    QElapsedTimer m_lastCountFetch;          // 最小间隔（60s）节流
    QElapsedTimer m_lastAutoRefresh;         // 自动刷新（refreshIfStale）的最小间隔节流

    // 内部 worker（类外定义，见本文件上方）：实际执行网络请求，存活于 m_thread
    DeviceWorker *m_worker = nullptr;
};
