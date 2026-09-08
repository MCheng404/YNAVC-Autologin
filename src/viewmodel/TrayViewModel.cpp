#include "TrayViewModel.h"
#include "model/AuthEngine.h"
#include "model/Settings.h"
#include "model/Scheduler.h"
#include "model/Logger.h"
#include "platform/NetworkAdapter.h"
#include "platform/Registry.h"

#include <QDesktopServices>
#include <QUrl>
#include <QApplication>
#include <QProcess>
#include <QStandardPaths>

TrayViewModel::TrayViewModel(AuthEngine *authEngine, Settings *settings,
                               Scheduler *scheduler, QObject *parent)
    : QObject(parent)
    , m_authEngine(authEngine)
    , m_settings(settings)
    , m_scheduler(scheduler)
{
    // 记录应用启动时间戳（C++ 层早于 QML 执行，确保值正确）
    m_appStartMs = QDateTime::currentMSecsSinceEpoch();

    m_autostartChecked = m_settings->autostart();

    // 监听自启状态变化
    connect(m_settings, &Settings::autostartChanged, this, [this]() {
        setAutostartChecked(m_settings->autostart());
    });

    // 监听认证成功 → 更新上次认证时间戳
    connect(m_authEngine, &AuthEngine::authCompleted, this, [this](AuthEngine::AuthResult result) {
        Q_UNUSED(result)
        if (result == AuthEngine::AuthResult::Success ||
            result == AuthEngine::AuthResult::AlreadyOnline) {
            m_lastAuthMs = m_authEngine->lastAuthMs();
            emit lastAuthMsChanged();
        }
    });
}

void TrayViewModel::onTrayClicked()
{
    if (m_settings->clickToAuth()) {
        if (!m_authEngine->isBusy()) {
            onLogin();
        }
    }
}

void TrayViewModel::onShowLog()
{
    // 使用记事本打开日志文件
    // 这里简化处理，实际可通过 Logger 获取路径
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                     + QStringLiteral("/logs/");
    QDesktopServices::openUrl(QUrl::fromLocalFile(logDir));
}

void TrayViewModel::onLogin()
{
    if (m_authEngine->isBusy()) return;

    setIconSource(QStringLiteral("authenticating"));
    setTooltip(QStringLiteral("校园网自动登录 - 认证中..."));

    QString mac = Platform::NetworkAdapter::getMacAddress();
    QString ip  = Platform::NetworkAdapter::getLocalIp(
        QStringLiteral("172.30.255.2"));

    if (!ip.isEmpty()) {
        m_authEngine->authenticate(mac, ip);
    }
}

void TrayViewModel::onCleanLog()
{
    emit cleanLogRequested();
}

void TrayViewModel::onToggleAutostart()
{
    bool newState = !m_autostartChecked;
    m_settings->setAutostart(newState);
    m_settings->save();
    setAutostartChecked(newState);
}

void TrayViewModel::onOpenSettings()
{
    qDebug() << "TrayViewModel: onOpenSettings() 被调用";
    emit openSettingsRequested();
    qDebug() << "TrayViewModel: openSettingsRequested() 信号已发出";
}

void TrayViewModel::onQuit()
{
    QApplication::quit();
}

void TrayViewModel::onAuthStateChanged(int state)
{
    switch (state) {
    case 0: // Connected
        setIconSource(QStringLiteral("connected"));
        setTooltip(QStringLiteral("校园网自动登录 - 已联网"));
        break;
    case 1: // Disconnected
        setIconSource(QStringLiteral("disconnected"));
        setTooltip(QStringLiteral("校园网自动登录 - 未联网"));
        break;
    case 2: // Authenticating
        setIconSource(QStringLiteral("authenticating"));
        setTooltip(QStringLiteral("校园网自动登录 - 认证中..."));
        break;
    default:
        break;
    }
}

qint64 TrayViewModel::nextAuthSec() const
{
    if (!m_settings->schedEnabled())
        return -1;
    return m_scheduler ? m_scheduler->nextTriggerTime() : 0;
}

void TrayViewModel::setIconSource(const QString &source)
{
    if (m_iconSource == source) return;
    m_iconSource = source;
    emit iconChanged();
}

void TrayViewModel::setTooltip(const QString &tip)
{
    if (m_tooltip == tip) return;
    m_tooltip = tip;
    emit tooltipChanged();
}

void TrayViewModel::setAutostartChecked(bool checked)
{
    if (m_autostartChecked == checked) return;
    m_autostartChecked = checked;
    emit autostartCheckedChanged();
}
