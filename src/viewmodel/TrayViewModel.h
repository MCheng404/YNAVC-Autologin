#pragma once

#include <QObject>
#include <QString>
#include "platform/SystemTray.h"

class AuthEngine;
class Settings;
class Scheduler;

/**
 * @brief 托盘图标/菜单状态暴露给 QML
 *
 * 处理 SystemTray 信号，触发认证/设置/退出等操作
 * 暴露图标状态、tooltip 等属性
 */
class TrayViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString iconSource READ iconSource NOTIFY iconChanged)
    Q_PROPERTY(QString tooltip READ tooltip NOTIFY tooltipChanged)
    Q_PROPERTY(bool autostartChecked READ autostartChecked NOTIFY autostartCheckedChanged)
    Q_PROPERTY(qint64 lastAuthMs READ lastAuthMs NOTIFY lastAuthMsChanged)
    Q_PROPERTY(qint64 appStartMs READ appStartMs CONSTANT)

    /** 下次定时认证时间戳（秒，UTC）。定时未启用时返回 -1 */
    Q_PROPERTY(qint64 nextAuthSec READ nextAuthSec CONSTANT)

public:
    explicit TrayViewModel(AuthEngine *authEngine, Settings *settings,
                            Scheduler *scheduler, QObject *parent = nullptr);

    QString iconSource() const { return m_iconSource; }
    QString tooltip() const { return m_tooltip; }
    bool autostartChecked() const { return m_autostartChecked; }
    qint64 lastAuthMs() const { return m_lastAuthMs; }
    qint64 appStartMs() const { return m_appStartMs; }
    qint64 nextAuthSec() const;

    // QML 可调用方法
    Q_INVOKABLE void onTrayClicked();
    Q_INVOKABLE void onShowLog();
    Q_INVOKABLE void onLogin();
    Q_INVOKABLE void onCleanLog();
    Q_INVOKABLE void onToggleAutostart();
    Q_INVOKABLE void onOpenSettings();
    Q_INVOKABLE void onQuit();

public slots:
    /** 认证状态变化回调（0=Connected, 1=Disconnected, 2=Authenticating） */
    void onAuthStateChanged(int state);

signals:
    void iconChanged();
    void tooltipChanged();
    void autostartCheckedChanged();
    void lastAuthMsChanged();

    /** 请求清理日志 */
    void cleanLogRequested();

    /** 请求打开设置窗口（QML 直接监听） */
    void openSettingsRequested();

    /** 请求手动登录（投递到 worker 线程执行，避免冻结 UI） */
    void manualLoginRequested();

private:
    void setIconSource(const QString &source);
    void setTooltip(const QString &tip);
    void setAutostartChecked(bool checked);

    AuthEngine *m_authEngine = nullptr;
    Settings   *m_settings   = nullptr;
    Scheduler  *m_scheduler  = nullptr;

    QString m_iconSource = QStringLiteral("disconnected");
    QString m_tooltip    = QStringLiteral("校园网自动登录 - 监控中");
    bool    m_autostartChecked = true;
    qint64  m_lastAuthMs = 0;
    qint64  m_appStartMs = 0;
};
