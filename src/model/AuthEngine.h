#pragma once

#include <QObject>
#include <QString>
#include <atomic>

class ConnectivityChecker;
class Logger;
class Settings;

/**
 * @brief 认证引擎：防抖 + 黑名单 + 退避 + 二次校验
 *
 * 核心认证逻辑：
 * 1. 原子锁防抖（std::atomic<bool> m_busy）
 * 2. 3秒时间窗口防抖
 * 3. 黑名单时段检测（03:55-05:05）
 * 4. HTTP POST 认证
 * 5. 二次校验（认证后等1s再次检测连通性）
 * 6. 指数退避
 */
class AuthEngine : public QObject {
    Q_OBJECT

public:
    /** 认证结果枚举 */
    enum class AuthResult {
        Success,        ///< 认证成功
        AlreadyOnline,  ///< 已在线
        Failed,         ///< 认证失败
        Busy,           ///< 认证进行中（防抖）
        Blackout        ///< 黑名单时段
    };
    Q_ENUM(AuthResult)

    explicit AuthEngine(ConnectivityChecker *checker, Logger *logger,
                        Settings *settings, QObject *parent = nullptr);

    /** 执行认证（线程安全，内部有防抖检查） */
    AuthResult authenticate(const QString &mac, const QString &ip);

    /** 是否正在认证中 */
    bool isBusy() const;

    /** 请求网关释放本机会话（GET http://172.30.255.2/F.htm）。
     *  用于连续认证失败后清理可能残留的脏会话，再重新认证。
     *  返回是否成功收到响应。 */
    bool logoutSession();

    /** 当前是否在黑名单时段（03:55-05:05） */
    bool isInBlackout() const;

    /** 重置失败计数 */
    void resetFailCount();

    /** 计算指数退避等待秒数 */
    int getExponentialBackoff(int fails) const;

    /** 上次认证成功的时间戳（毫秒），0=从未成功 */
    qint64 lastAuthMs() const { return m_lastAuthMs.load(); }

signals:
    /** 认证完成信号 */
    void authCompleted(AuthResult result);

    /** 状态消息信号 */
    void statusMessage(const QString &msg);

private:
    /** 构建 POST 数据 */
    QString buildPostData(const QString &username, const QString &password,
                          const QString &ip, const QString &mac) const;

    /** 清理 MAC 地址（去冒号/横杠，转大写） */
    static QString cleanMac(const QString &mac);

private:
    std::atomic<bool>  m_busy{false};
    std::atomic<qint64> m_lastAuthMs{0};
    int                m_failCount{0};

    ConnectivityChecker *m_checker  = nullptr;
    Logger              *m_logger   = nullptr;
    Settings            *m_settings = nullptr;
    // QNetworkAccessManager 不做类成员：authenticate() 可能被工作线程调用，
    // QNAM 必须在使用它的线程中创建（在函数内创建局部 QNAM）
};
