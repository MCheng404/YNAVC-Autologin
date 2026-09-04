#include "Settings.h"

Settings::Settings(QObject *parent)
    : QObject(parent)
{
}

void Settings::load()
{
    const QString path = QString::fromUtf8(kRegPath);

    auto readBool = [&](const char *name, bool defaultValue) -> bool {
        auto val = Platform::Registry::readDword(path, QString::fromUtf8(name));
        return val.has_value() ? (val.value() != 0) : defaultValue;
    };

    auto readInt = [&](const char *name, int defaultValue) -> int {
        auto val = Platform::Registry::readDword(path, QString::fromUtf8(name));
        return val.has_value() ? static_cast<int>(val.value()) : defaultValue;
    };

    m_clickToAuth   = readBool("ClickToAuth", true);
    m_autostart     = readBool("AutoStart", true);
    m_notifications = readBool("Notifications", true);
    m_autoHotspot   = readBool("AutoHotspot", false);
    m_highPriority  = readBool("HighPriority", false);
    m_schedEnabled  = readBool("SchedEnabled", false);
    m_schedInterval = readInt("SchedInterval", 3);
    m_theme         = readInt("Theme", 0);

    // 范围校验
    if (m_schedInterval < 1) m_schedInterval = 1;
    if (m_schedInterval > 24) m_schedInterval = 24;

    // 读取字符串值
    auto username = Platform::Registry::readString(path, QStringLiteral("Username"));
    if (username.has_value()) m_username = username.value();

    auto password = Platform::Registry::readString(path, QStringLiteral("Password"));
    if (password.has_value()) m_password = password.value();

    m_accentColor = readInt("AccentColor", 0);
    if (m_accentColor < 0) m_accentColor = 0;
    if (m_accentColor > 15) m_accentColor = 0;

    m_toastPosition = readInt("ToastPosition", 0);
    if (m_toastPosition < 0) m_toastPosition = 0;
    if (m_toastPosition > 2) m_toastPosition = 0;

    // 检查自启注册表状态
    m_autostart = Platform::Registry::isAutostartEnabled();

    emit settingsChanged();
}

void Settings::save()
{
    const QString path = QString::fromUtf8(kRegPath);

    Platform::Registry::writeDword(path, QStringLiteral("ClickToAuth"),
                                    m_clickToAuth ? 1 : 0);
    Platform::Registry::writeDword(path, QStringLiteral("AutoStart"),
                                    m_autostart ? 1 : 0);
    Platform::Registry::writeDword(path, QStringLiteral("Notifications"),
                                    m_notifications ? 1 : 0);
    Platform::Registry::writeDword(path, QStringLiteral("AutoHotspot"),
                                    m_autoHotspot ? 1 : 0);
    Platform::Registry::writeDword(path, QStringLiteral("HighPriority"),
                                    m_highPriority ? 1 : 0);
    Platform::Registry::writeDword(path, QStringLiteral("SchedEnabled"),
                                    m_schedEnabled ? 1 : 0);
    Platform::Registry::writeDword(path, QStringLiteral("SchedInterval"),
                                    static_cast<DWORD>(m_schedInterval));
    Platform::Registry::writeDword(path, QStringLiteral("Theme"),
                                    static_cast<DWORD>(m_theme));
    Platform::Registry::writeDword(path, QStringLiteral("AccentColor"),
                                    static_cast<DWORD>(m_accentColor));
    Platform::Registry::writeDword(path, QStringLiteral("ToastPosition"),
                                    static_cast<DWORD>(m_toastPosition));

    Platform::Registry::writeString(path, QStringLiteral("Username"), m_username);
    Platform::Registry::writeString(path, QStringLiteral("Password"), m_password);

    // 开机自启
    Platform::Registry::setAutostart(m_autostart);

    emit settingsChanged();
}

// === Setters ===

void Settings::setClickToAuth(bool v) {
    if (m_clickToAuth == v) return;
    m_clickToAuth = v;
    emit clickToAuthChanged();
    emit settingsChanged();
}

void Settings::setAutostart(bool v) {
    if (m_autostart == v) return;
    m_autostart = v;
    emit autostartChanged();
    emit settingsChanged();
}

void Settings::setNotifications(bool v) {
    if (m_notifications == v) return;
    m_notifications = v;
    emit notificationsChanged();
    emit settingsChanged();
}

void Settings::setAutoHotspot(bool v) {
    if (m_autoHotspot == v) return;
    m_autoHotspot = v;
    emit autoHotspotChanged();
    emit settingsChanged();
}

void Settings::setHighPriority(bool v) {
    if (m_highPriority == v) return;
    m_highPriority = v;
    emit highPriorityChanged();
    emit settingsChanged();
}

void Settings::setSchedEnabled(bool v) {
    if (m_schedEnabled == v) return;
    m_schedEnabled = v;
    emit schedEnabledChanged();
    emit settingsChanged();
}

void Settings::setSchedInterval(int v) {
    v = qBound(1, v, 24);
    if (m_schedInterval == v) return;
    m_schedInterval = v;
    emit schedIntervalChanged();
    emit settingsChanged();
}

void Settings::setUsername(const QString &v) {
    if (m_username == v) return;
    m_username = v;
    emit usernameChanged();
    emit settingsChanged();
}

void Settings::setPassword(const QString &v) {
    if (m_password == v) return;
    m_password = v;
    emit passwordChanged();
    emit settingsChanged();
}

void Settings::setTheme(int v) {
    if (m_theme == v) return;
    m_theme = v;
    emit themeChanged();
    emit settingsChanged();
}

void Settings::setAccentColor(int v) {
    v = qBound(0, v, 15);
    if (m_accentColor == v) return;
    m_accentColor = v;
    emit accentColorChanged();
    emit settingsChanged();
}

void Settings::setToastPosition(int v) {
    v = qBound(0, v, 2);
    if (m_toastPosition == v) return;
    m_toastPosition = v;
    emit toastPositionChanged();
    emit settingsChanged();
}
