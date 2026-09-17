#include "AuthViewModel.h"
#include "model/AuthEngine.h"
#include "model/ConnectivityChecker.h"
#include "platform/NetworkAdapter.h"

AuthViewModel::AuthViewModel(AuthEngine *authEngine, QObject *parent)
    : QObject(parent)
    , m_authEngine(authEngine)
{
    m_statusText = QStringLiteral("监控中");
}

void AuthViewModel::triggerLogin()
{
    if (m_authEngine->isBusy()) return;

    setAuthState(AuthState::Authenticating);
    setStatusText(QStringLiteral("认证中..."));

    QString ip  = Platform::NetworkAdapter::getLocalIp(
        QStringLiteral("172.30.255.2"));

    if (!ip.isEmpty()) {
        // 投递到 worker 线程执行认证，避免冻结 UI 线程
        emit manualLoginRequested();
    } else {
        setStatusText(QStringLiteral("无法获取IP地址"));
        setAuthState(AuthState::Disconnected);
    }
}

void AuthViewModel::checkConnectivity()
{
    setStatusText(QStringLiteral("检测连通性..."));
}

void AuthViewModel::onAuthCompleted(AuthEngine::AuthResult result)
{
    switch (result) {
    case AuthEngine::AuthResult::Success:
        setAuthState(AuthState::Connected);
        setStatusText(QStringLiteral("已联网"));
        break;
    case AuthEngine::AuthResult::AlreadyOnline:
        setAuthState(AuthState::Connected);
        setStatusText(QStringLiteral("已在线"));
        break;
    case AuthEngine::AuthResult::Failed:
        setAuthState(AuthState::Disconnected);
        setStatusText(QStringLiteral("认证失败"));
        break;
    case AuthEngine::AuthResult::Busy:
        setAuthState(AuthState::Idle);
        setStatusText(QStringLiteral("认证进行中"));
        break;
    case AuthEngine::AuthResult::Blackout:
        setAuthState(AuthState::Idle);
        setStatusText(QStringLiteral("禁止认证时段"));
        break;
    }
}

void AuthViewModel::onStatusMessage(const QString &msg)
{
    setStatusText(msg);
}

void AuthViewModel::setAuthState(AuthState state)
{
    if (m_authState == state) return;
    m_authState = state;
    emit authStateChanged();
    emit isAuthenticatingChanged();
}

void AuthViewModel::setStatusText(const QString &text)
{
    if (m_statusText == text) return;
    m_statusText = text;
    emit statusTextChanged();
}
