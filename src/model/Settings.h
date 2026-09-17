#pragma once

#include <QObject>
#include <QString>
#include <QReadWriteLock>   // 提供 QReadWriteLock / QWriteLocker / QReadLocker
#include "platform/Registry.h"

/**
 * @brief 设置模型（Q_PROPERTY + 注册表持久化）
 *
 * 所有设置项通过 Q_PROPERTY 暴露给 ViewModel/QML
 * 注册表键路径：HKCU\Software\AutoLogin\Settings
 * 自启键：HKCU\Software\Microsoft\Windows\CurrentVersion\Run\AutoLogin
 */
class Settings : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool clickToAuth READ clickToAuth WRITE setClickToAuth NOTIFY clickToAuthChanged)
    Q_PROPERTY(bool autostart READ autostart WRITE setAutostart NOTIFY autostartChanged)
    Q_PROPERTY(bool notifications READ notifications WRITE setNotifications NOTIFY notificationsChanged)
    Q_PROPERTY(bool autoHotspot READ autoHotspot WRITE setAutoHotspot NOTIFY autoHotspotChanged)
    Q_PROPERTY(bool highPriority READ highPriority WRITE setHighPriority NOTIFY highPriorityChanged)
    Q_PROPERTY(bool schedEnabled READ schedEnabled WRITE setSchedEnabled NOTIFY schedEnabledChanged)
    Q_PROPERTY(int schedInterval READ schedInterval WRITE setSchedInterval NOTIFY schedIntervalChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(int theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(int accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(int toastPosition READ toastPosition WRITE setToastPosition NOTIFY toastPositionChanged)

public:
    explicit Settings(QObject *parent = nullptr);

    /** 从注册表加载所有设置 */
    void load();

    /** 保存所有设置到注册表 */
    void save();

    // === Getters ===
    bool clickToAuth() const { return m_clickToAuth; }
    bool autostart() const { return m_autostart; }
    bool notifications() const { return m_notifications; }
    bool autoHotspot() const { return m_autoHotspot; }
    bool highPriority() const { return m_highPriority; }
    bool schedEnabled() const { return m_schedEnabled; }
    int schedInterval() const { return m_schedInterval; }
    QString username() const { return m_username; }
    QString password() const { return m_password; }
    int theme() const { return m_theme; }
    int accentColor() const { return m_accentColor; }
    int toastPosition() const { return m_toastPosition; }

    // === Setters ===
    void setClickToAuth(bool v);
    void setAutostart(bool v);
    void setNotifications(bool v);
    void setAutoHotspot(bool v);
    void setHighPriority(bool v);
    void setSchedEnabled(bool v);
    void setSchedInterval(int v);
    void setUsername(const QString &v);
    void setPassword(const QString &v);
    void setTheme(int v);
    void setAccentColor(int v);
    void setToastPosition(int v);

signals:
    void clickToAuthChanged();
    void autostartChanged();
    void notificationsChanged();
    void autoHotspotChanged();
    void highPriorityChanged();
    void schedEnabledChanged();
    void schedIntervalChanged();
    void usernameChanged();
    void passwordChanged();
    void themeChanged();
    void accentColorChanged();
    void toastPositionChanged();
    void settingsChanged();

private:
    static constexpr const char *kRegPath = "Software\\AutoLogin\\Settings";

    bool    m_clickToAuth   = true;
    bool    m_autostart     = true;
    bool    m_notifications = true;
    bool    m_autoHotspot   = false;
    bool    m_highPriority  = false;
    bool    m_schedEnabled  = false;
    int     m_schedInterval = 3;
    QString m_username;
    QString m_password;
    int     m_theme         = 2; // 0=Dark, 1=Light, 2=System
    int     m_accentColor   = 0;  // 0-15 主题色索引（默认粉色 #ec4899）
    int     m_toastPosition = 0;  // 0=右下, 1=左下, 2=顶部居中

    mutable QReadWriteLock m_lock;  // 保护设置读写，防止 worker/UI 线程 QString 数据竞争
};
