#pragma once

#include <QObject>
#include <QThread>
#include <QWindow>

// 前向声明
class AuthEngine;
class ConnectivityChecker;
class Scheduler;
class Logger;
class Settings;
class AuthViewModel;
class SettingsViewModel;
class TrayViewModel;
class ThemeViewModel;

namespace Platform {
class SystemTray;
class HotspotManager;
}

/**
 * @brief 应用编排器 — 创建 Model/VM/Tray，组装 MVVM 管线
 *
 * 生命周期：
 * 1. initialize() — 创建所有对象，加载设置，启动 Worker Thread
 * 2. run()        — 进入 Qt 事件循环
 * 3. 析构         — m_running=false，等待 Worker Thread 结束，清理资源
 */
class App : public QObject {
    Q_OBJECT

public:
    explicit App(QObject *parent = nullptr);
    ~App();

    /** 初始化所有组件，返回 true 表示成功 */
    bool initialize();

    /** 进入 Qt 事件循环，返回退出码 */
    int run();

    /** 应用圆角矩形窗口遮罩（Windows API）
     *  供 QML（Main.qml 的 Loader.onLoaded）调用。
     *  注意：必须放在 public 段——private 的 Q_INVOKABLE 对 QML 不可见：
     *  moc 仍会生成元对象条目，但 QML 引擎只暴露 public 方法，
     *  结果 typeof app.applyWindowMask === 'undefined'。
     */
    Q_INVOKABLE void applyWindowMask(QWindow *win, int radius);

private:
    /** 启动 Worker Thread（认证主循环） */
    void setupWorkerThread();

    /** 连接 SystemTray 信号到 TrayViewModel */
    void setupTrayConnections();

    /** 连接 Model ↔ ViewModel ↔ View 的信号/槽 */
    void setupViewModelBindings();

    /** Worker Thread 主循环（在子线程中运行） */
    void workerLoop();

    /** 可中断睡眠：每秒检查 m_running，可被析构时的 m_running=false 打断 */
    void interruptibleSleep(int seconds);

    /** 扫描所有 QWindow 并应用圆角遮罩 */
    void applyMasksToAllWindows();

private:
    // Model 层
    AuthEngine          *m_authEngine     = nullptr;
    ConnectivityChecker *m_connectivity   = nullptr;
    Scheduler           *m_scheduler      = nullptr;
    Logger              *m_logger         = nullptr;
    Settings            *m_settings       = nullptr;

    // ViewModel 层
    AuthViewModel       *m_authVM        = nullptr;
    SettingsViewModel   *m_settingsVM     = nullptr;
    TrayViewModel       *m_trayVM        = nullptr;
    ThemeViewModel      *m_themeVM       = nullptr;

    // Platform 层
    Platform::SystemTray     *m_tray     = nullptr;
    Platform::HotspotManager *m_hotspot  = nullptr;

    // Worker Thread
    QThread            *m_workerThread   = nullptr;
    std::atomic<bool>   m_running{false};
    std::atomic<bool>   m_manualAuthRequested{false};  // UI 线程置位，worker 线程轮询执行手动登录
};
