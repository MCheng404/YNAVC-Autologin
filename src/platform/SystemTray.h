#pragma once

#include <QSystemTrayIcon>
#include <QMenu>

namespace Platform {

/**
 * @brief QSystemTrayIcon + QMenu 封装
 *
 * Qt6 无原生 QML SystemTray，使用 C++ 端 QSystemTrayIcon
 * 通过信号/槽与 TrayViewModel 通信
 */
class SystemTray : public QSystemTrayIcon {
    Q_OBJECT

public:
    /** 托盘图标状态 */
    enum class Status {
        Connected,
        Disconnected,
        Authenticating
    };
    Q_ENUM(Status)

    explicit SystemTray(QObject *parent = nullptr);
    ~SystemTray();

    /** 更新托盘图标 */
    void updateIcon(Status status);

    /** 更新 tooltip 文字（QVariant 重载，供信号连接） */
    void updateTooltip(const QVariant &tooltip);

    /** 显示托盘图标 */
    void show();

    /** 获取内部 QSystemTrayIcon 指针（供 Notification 使用） */
    QSystemTrayIcon *trayIcon() const { return const_cast<SystemTray*>(this); }

signals:
    /** 用户请求显示日志 */
    void showLogRequested();

    /** 用户请求立即登录 */
    void loginRequested();

    /** 用户请求清理日志 */
    void cleanLogRequested();

    /** 用户请求切换开机自启 */
    void toggleAutostartRequested();

    /** 用户请求打开设置窗口 */
    void openSettingsRequested();

    /** 用户请求退出 */
    void quitRequested();

    /** 托盘图标被点击（左键） */
    void trayClicked();

private:
    /** 构建右键菜单 */
    void rebuildMenu();

    /** 状态枚举 → 菜单文案（如"状态：已连接"） */
    QString statusLabel() const;

    QMenu *m_menu = nullptr;
    Status m_status = Status::Disconnected;
};

} // namespace Platform
