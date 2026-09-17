#pragma once

#include <QObject>
#include <QVariantList>
#include <QThread>
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
    Q_INVOKABLE void kick(const QString &sessionId);      // 踢单个
    Q_INVOKABLE void kickAllExceptSelf();                 // 踢掉除本机 MAC 之外的全部

    // 工具函数。放在 public：DeviceWorker 定义在类外，需要调用它们。
    static QString maskAccount(const QString &account);
    static QString normalizeMac(const QString &mac);

signals:
    void devicesChanged();
    void loadingChanged();
    void errorMessageChanged();
    void accountChanged();

private slots:
    // 由 worker 线程通过 QueuedConnection 回调，运行在 UI 线程
    void onFetchResult(const QVariantList &devices, const QString &error);
    void onKickResult(bool ok, const QString &error, const QString &sessionId);
    void onKickAllResult(int total, int success, int failed, const QString &error);

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

    // 内部 worker（类外定义，见本文件上方）：实际执行网络请求，存活于 m_thread
    DeviceWorker *m_worker = nullptr;
};
