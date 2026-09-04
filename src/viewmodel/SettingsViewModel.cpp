#include "SettingsViewModel.h"

SettingsViewModel::SettingsViewModel(Settings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    // 转发 Settings 的所有 Changed 信号
    connect(m_settings, &Settings::clickToAuthChanged,
            this, &SettingsViewModel::clickToAuthChanged);
    connect(m_settings, &Settings::autostartChanged,
            this, &SettingsViewModel::autostartChanged);
    connect(m_settings, &Settings::notificationsChanged,
            this, &SettingsViewModel::notificationsChanged);
    connect(m_settings, &Settings::autoHotspotChanged,
            this, &SettingsViewModel::autoHotspotChanged);
    connect(m_settings, &Settings::highPriorityChanged,
            this, &SettingsViewModel::highPriorityChanged);
    connect(m_settings, &Settings::schedEnabledChanged,
            this, &SettingsViewModel::schedEnabledChanged);
    connect(m_settings, &Settings::schedIntervalChanged,
            this, &SettingsViewModel::schedIntervalChanged);
    connect(m_settings, &Settings::usernameChanged,
            this, &SettingsViewModel::usernameChanged);
    connect(m_settings, &Settings::passwordChanged,
            this, &SettingsViewModel::passwordChanged);
}

void SettingsViewModel::save()
{
    m_settings->save();
    emit settingsSaved();
}

void SettingsViewModel::load()
{
    m_settings->load();
}

void SettingsViewModel::resetDefaults()
{
    m_settings->setClickToAuth(true);
    m_settings->setAutostart(true);
    m_settings->setNotifications(true);
    m_settings->setAutoHotspot(false);
    m_settings->setHighPriority(false);
    m_settings->setSchedEnabled(false);
    m_settings->setSchedInterval(3);
    m_settings->setUsername(QString());
    m_settings->setPassword(QString());
    m_settings->save();
}
