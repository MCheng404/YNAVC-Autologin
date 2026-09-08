#include "SystemTray.h"
#include "Registry.h"

#include <QApplication>
#include <QAction>
#include <QCursor>

namespace Platform {

SystemTray::SystemTray(QObject *parent)
    : QSystemTrayIcon(parent)
{
    m_menu = new QMenu();

    // 设置默认图标（断网状态）
    updateIcon(Status::Disconnected);

    // 右键菜单
    rebuildMenu();

    setContextMenu(m_menu);

    // 左键点击
    connect(this, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            emit trayClicked();
        }
    });
}

SystemTray::~SystemTray()
{
    delete m_menu;
}

void SystemTray::updateIcon(Status status)
{
    m_status = status;
    QString iconName;
    switch (status) {
    case Status::Connected:
        iconName = QStringLiteral("connected");
        break;
    case Status::Disconnected:
        iconName = QStringLiteral("disconnected");
        break;
    case Status::Authenticating:
        iconName = QStringLiteral("authenticating");
        break;
    }

    setIcon(QIcon(QStringLiteral(":/AutoLogin/resources/icons/%1.svg").arg(iconName)));

    // 状态变化后刷新菜单顶部状态项文案/图标
    rebuildMenu();
}

void SystemTray::updateTooltip(const QVariant &tooltip)
{
    setToolTip(tooltip.toString());
}

void SystemTray::show()
{
    QSystemTrayIcon::show();
}

void SystemTray::rebuildMenu()
{
    m_menu->clear();

    bool autostart = Registry::isAutostartEnabled();

    // 顶部状态项（disabled，仅展示当前状态）
    QAction *statusAction = m_menu->addAction(statusLabel());
    statusAction->setEnabled(false);
    QString statusIcon;
    switch (m_status) {
    case Status::Connected:     statusIcon = QStringLiteral("connected"); break;
    case Status::Authenticating: statusIcon = QStringLiteral("authenticating"); break;
    case Status::Disconnected:   statusIcon = QStringLiteral("disconnected"); break;
    }
    statusAction->setIcon(
        QIcon(QStringLiteral(":/AutoLogin/resources/icons/%1.svg").arg(statusIcon)));

    m_menu->addSeparator();

    QAction *showLogAction = m_menu->addAction(QStringLiteral("显示日志"));
    connect(showLogAction, &QAction::triggered, this, &SystemTray::showLogRequested);

    QAction *loginAction = m_menu->addAction(QStringLiteral("立即登录"));
    connect(loginAction, &QAction::triggered, this, &SystemTray::loginRequested);

    QAction *cleanLogAction = m_menu->addAction(QStringLiteral("清理日志"));
    connect(cleanLogAction, &QAction::triggered, this, &SystemTray::cleanLogRequested);

    m_menu->addSeparator();

    QAction *autostartAction = m_menu->addAction(QStringLiteral("开机自启"));
    autostartAction->setCheckable(true);
    autostartAction->setChecked(autostart);
    connect(autostartAction, &QAction::triggered, this, &SystemTray::toggleAutostartRequested);

    m_menu->addSeparator();

    QAction *settingsAction = m_menu->addAction(QStringLiteral("设置..."));
    connect(settingsAction, &QAction::triggered, this, [this]() {
        qDebug() << "SystemTray: 设置菜单被点击";
        emit openSettingsRequested();
    });

    m_menu->addSeparator();

    QAction *quitAction = m_menu->addAction(QStringLiteral("退出"));
    connect(quitAction, &QAction::triggered, this, &SystemTray::quitRequested);
}

QString SystemTray::statusLabel() const
{
    switch (m_status) {
    case Status::Connected:     return QStringLiteral("状态：已连接");
    case Status::Authenticating: return QStringLiteral("状态：认证中");
    case Status::Disconnected:   return QStringLiteral("状态：未连接");
    }
    return QStringLiteral("状态：未连接");
}

} // namespace Platform
