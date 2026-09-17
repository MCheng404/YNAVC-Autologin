#pragma once

#include <QObject>
#include <QString>
#include "model/AuthEngine.h"

/**
 * @brief 认证状态暴露给 QML
 *
 * 暴露属性：
 * - authState: 认证状态枚举（Idle/Connected/Disconnected/Authenticating）
 * - statusText: 人类可读的状态文本
 * - isAuthenticating: 是否正在认证中
 */
class AuthViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(AuthState authState READ authState NOTIFY authStateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool isAuthenticating READ isAuthenticating NOTIFY isAuthenticatingChanged)

public:
    /** 认证状态枚举（暴露给 QML） */
    enum class AuthState {
        Idle = 0,
        Connected,
        Disconnected,
        Authenticating
    };
    Q_ENUM(AuthState)

    explicit AuthViewModel(AuthEngine *authEngine, QObject *parent = nullptr);

    AuthState authState() const { return m_authState; }
    QString statusText() const { return m_statusText; }
    bool isAuthenticating() const { return m_authState == AuthState::Authenticating; }

    /** QML 调用：触发登录 */
    Q_INVOKABLE void triggerLogin();

    /** QML 调用：检测连通性 */
    Q_INVOKABLE void checkConnectivity();

public slots:
    /** 认证完成回调 */
    void onAuthCompleted(AuthEngine::AuthResult result);

    /** 状态消息回调 */
    void onStatusMessage(const QString &msg);

signals:
    void authStateChanged();
    void statusTextChanged();
    void isAuthenticatingChanged();

    /** 请求手动登录（投递到 worker 线程执行，避免冻结 UI） */
    void manualLoginRequested();

private:
    void setAuthState(AuthState state);
    void setStatusText(const QString &text);

    AuthEngine *m_authEngine = nullptr;
    AuthState   m_authState = AuthState::Idle;
    QString     m_statusText;
};
