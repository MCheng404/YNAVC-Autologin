#include "App.h"
#include "model/AuthEngine.h"
#include "model/ConnectivityChecker.h"
#include "model/Scheduler.h"
#include "model/Logger.h"
#include "model/Settings.h"
#include "viewmodel/AuthViewModel.h"
#include "viewmodel/SettingsViewModel.h"
#include "viewmodel/TrayViewModel.h"
#include "viewmodel/ThemeViewModel.h"
#include "platform/SystemTray.h"
#include "platform/HotspotManager.h"
#include "platform/NetworkAdapter.h"
#include "platform/Notification.h"
#include "platform/ToastWindow.h"

#include <QApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QMetaObject>
#include <QTimer>
#include <QDateTime>
#include <QFontDatabase>
#include <QFont>
#include <QWindow>
#include <QRasterWindow>
#include <QFile>
#include <QTime>

#ifdef _WIN32
#include <windows.h>
#endif

App::App(QObject *parent)
    : QObject(parent)
{
}

App::~App()
{
    m_running = false;

    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait(8000);
        delete m_workerThread;
        m_workerThread = nullptr;
    }

    // 释放顺序：ViewModel → Model → Platform
    delete m_trayVM;
    delete m_themeVM;
    delete m_settingsVM;
    delete m_authVM;

    delete m_scheduler;
    delete m_connectivity;
    delete m_authEngine;

    delete m_tray;
    delete m_hotspot;
    delete m_logger;

    // Settings 最后释放（其他对象可能仍引用）
    delete m_settings;
}

bool App::initialize()
{
    // 0. 加载内置字体（霞鹜 + Inter + JetBrains Mono）
    {
        auto loadFont = [](const QString &path) {
            int id = QFontDatabase::addApplicationFont(path);
            if (id >= 0) {
                QStringList families = QFontDatabase::applicationFontFamilies(id);
                if (!families.isEmpty()) {
                    qDebug() << "Loaded font:" << families.first() << "from" << path;
                }
            }
            return id;
        };

        // 霞鹜新晰黑 Plus（中文）
        loadFont(QStringLiteral(":/AutoLogin/resources/fonts/embedded/LXGWNeoXiHeiPlus.ttf"));

        // Inter Variable（英文）
        loadFont(QStringLiteral(":/AutoLogin/resources/fonts/embedded/InterVariable.ttf"));

        // JetBrains Mono（数字等宽）
        loadFont(QStringLiteral(":/AutoLogin/resources/fonts/embedded/JetBrainsMono-Regular.ttf"));
    }

    // 1. 创建 Model 层
    m_logger       = new Logger(this);
    m_settings     = new Settings(this);
    m_settings->load();

    m_connectivity = new ConnectivityChecker(this);
    m_scheduler    = new Scheduler(m_settings, this);
    m_authEngine   = new AuthEngine(m_connectivity, m_logger, m_settings, this);

    // 2. 创建 Platform 层
    m_hotspot = new Platform::HotspotManager();
    m_hotspot->setLogger(m_logger);
    m_tray    = new Platform::SystemTray();

    // 初始化通知
    Platform::Notification::setEnabled(m_settings->notifications());

    // 3. 创建 ViewModel 层
    m_authVM     = new AuthViewModel(m_authEngine, this);
    m_settingsVM = new SettingsViewModel(m_settings, this);
    m_trayVM     = new TrayViewModel(m_authEngine, m_settings, m_scheduler, this);
    m_themeVM    = new ThemeViewModel(m_settings, this);

    // 通知窗口可能在任何 QML 窗口创建前弹出，提前注入主题 ViewModel
    ToastWindow::setThemeViewModel(m_themeVM);

    // 4. 初始化日志
    m_logger->log(QStringLiteral("AutoLogin v2.13 启动"));

    // 5. 设置 ViewModel 绑定
    setupViewModelBindings();

    // 6. 设置托盘连接
    setupTrayConnections();

    // 7. 初始化通知系统（自定义 QWindow Toast，无系统 API 依赖）
    Platform::Notification::setEnabled(m_settings->notifications());

    // 8. 显示托盘图标
    m_tray->show();
    m_tray->updateTooltip(QVariant(QStringLiteral("校园网自动登录 - 监控中")));

    // 9. 启动通知
    if (m_settings->notifications()) {
        Platform::Notification::show(
            QStringLiteral("校园网登录"),
            QStringLiteral("程序已启动，后台监控中..."),
            Platform::Notification::Type::Info);
    }

    // 9. 启动 Worker Thread
    setupWorkerThread();

    // 10. 定时日志自动清理（每小时检查一次）
    QTimer *cleanupTimer = new QTimer(this);
    connect(cleanupTimer, &QTimer::timeout, m_logger, &Logger::autoCleanup);
    cleanupTimer->start(3600000); // 1 小时

    return true;
}

int App::run()
{
    // 加载 QML
    QQmlApplicationEngine engine;

    // 注入 ViewModel 到 QML 上下文
    engine.rootContext()->setContextProperty(QStringLiteral("authVM"),     m_authVM);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsVM"), m_settingsVM);
    engine.rootContext()->setContextProperty(QStringLiteral("trayVM"),     m_trayVM);
    engine.rootContext()->setContextProperty(QStringLiteral("themeVM"),    m_themeVM);
    engine.rootContext()->setContextProperty(QStringLiteral("app"),        this);  // 暴露给 QML 调用 applyWindowMask

    const QUrl url(QStringLiteral("qrc:/AutoLogin/src/view/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &engine, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    // 使用 singleShot 延迟执行，确保 exec() 启动后窗口已注册
    QTimer::singleShot(0, this, [this]() {
        applyMasksToAllWindows();
    });

    return QApplication::exec();
}

void App::setupWorkerThread()
{
    m_running = true;
    m_workerThread = QThread::create([this]() {
        workerLoop();
    });

    m_workerThread->start();

    // 线程启动后再设置优先级
    if (m_settings->highPriority()) {
        m_workerThread->setPriority(QThread::HighPriority);
    } else {
        m_workerThread->setPriority(QThread::LowestPriority);
    }
}

void App::setupTrayConnections()
{
    // SystemTray 信号 → TrayViewModel 槽
    connect(m_tray, &Platform::SystemTray::loginRequested,
            m_trayVM, &TrayViewModel::onLogin);
    connect(m_tray, &Platform::SystemTray::showLogRequested,
            m_trayVM, &TrayViewModel::onShowLog);
    connect(m_tray, &Platform::SystemTray::cleanLogRequested,
            m_trayVM, &TrayViewModel::onCleanLog);
    connect(m_tray, &Platform::SystemTray::toggleAutostartRequested,
            m_trayVM, &TrayViewModel::onToggleAutostart);
    connect(m_tray, &Platform::SystemTray::openSettingsRequested,
            m_trayVM, &TrayViewModel::onOpenSettings);
    qDebug() << "App: SystemTray::openSettingsRequested -> TrayViewModel::onOpenSettings 连接已建立";
    connect(m_tray, &Platform::SystemTray::quitRequested,
            m_trayVM, &TrayViewModel::onQuit);
    connect(m_tray, &Platform::SystemTray::trayClicked,
            m_trayVM, &TrayViewModel::onTrayClicked);

    // TrayViewModel iconSource → SystemTray::updateIcon（类型转换：QString → Status 枚举）
    connect(m_trayVM, &TrayViewModel::iconChanged, this, [this]() {
        const QString &src = m_trayVM->iconSource();
        Platform::SystemTray::Status status = Platform::SystemTray::Status::Disconnected;
        if (src == QStringLiteral("connected"))
            status = Platform::SystemTray::Status::Connected;
        else if (src == QStringLiteral("authenticating"))
            status = Platform::SystemTray::Status::Authenticating;
        m_tray->updateIcon(status);
    });

    // TrayViewModel tooltip → SystemTray::updateTooltip
    connect(m_trayVM, &TrayViewModel::tooltipChanged, this, [this]() {
        m_tray->updateTooltip(QVariant(m_trayVM->tooltip()));
    });
}

void App::setupViewModelBindings()
{
    // AuthEngine → AuthViewModel（跨线程安全更新）
    connect(m_authEngine, &AuthEngine::authCompleted,
            m_authVM, &AuthViewModel::onAuthCompleted);
    connect(m_authEngine, &AuthEngine::statusMessage,
            m_authVM, &AuthViewModel::onStatusMessage);

    // AuthViewModel → TrayViewModel（图标状态联动：枚举转 int）
    connect(m_authVM, &AuthViewModel::authStateChanged, this, [this]() {
        int state = 0; // Default: Connected
        switch (m_authVM->authState()) {
        case AuthViewModel::AuthState::Connected:      state = 0; break;
        case AuthViewModel::AuthState::Disconnected:   state = 1; break;
        case AuthViewModel::AuthState::Authenticating: state = 2; break;
        case AuthViewModel::AuthState::Idle:           state = 1; break;
        }
        m_trayVM->onAuthStateChanged(state);
    });

    // SettingsViewModel → Settings 保存
    connect(m_settingsVM, &SettingsViewModel::settingsSaved,
            m_settings, &Settings::save);

    // TrayViewModel → 日志清理（manualCleanup 返回 bool，用 lambda 包装）
    connect(m_trayVM, &TrayViewModel::cleanLogRequested, this, [this]() {
        m_logger->manualCleanup();
    });

    // AuthEngine → 通知（认证成功/失败时弹出气泡）
    connect(m_authEngine, &AuthEngine::authCompleted, this,
            [this](AuthEngine::AuthResult result) {
        if (!m_settings->notifications()) return;

        switch (result) {
        case AuthEngine::AuthResult::Success:
            Platform::Notification::show(
                QStringLiteral("校园网登录"),
                QStringLiteral("认证成功，已联网"),
                Platform::Notification::Type::Success);
            break;
        case AuthEngine::AuthResult::AlreadyOnline:
            Platform::Notification::show(
                QStringLiteral("校园网登录"),
                QStringLiteral("已在线，无需重复认证"),
                Platform::Notification::Type::Info);
            break;
        case AuthEngine::AuthResult::Failed:
            Platform::Notification::show(
                QStringLiteral("校园网登录"),
                QStringLiteral("认证失败，请检查网络"),
                Platform::Notification::Type::Error);
            break;
        default:
            break;
        }
    });

    // Settings → 通知开关同步
    connect(m_settings, &Settings::notificationsChanged, this, [this]() {
        Platform::Notification::setEnabled(m_settings->notifications());
    });
}

void App::workerLoop()
{
    m_logger->log(QStringLiteral("工作线程启动"));
    int fails = 0;
    bool prevAirplane = false;

    // 设置初始托盘图标状态
    QMetaObject::invokeMethod(m_tray, [this]() {
        m_tray->updateIcon(Platform::SystemTray::Status::Disconnected);
    }, Qt::QueuedConnection);

    while (m_running.load()) {
        // === 定时认证检查（仅在联网状态下触发） ===
        if (m_settings->schedEnabled()) {
            qint64 nowSec = QDateTime::currentSecsSinceEpoch();
            if (m_scheduler->shouldTrigger(nowSec)) {
                if (!m_authEngine->isBusy() && !m_authEngine->isInBlackout()) {
                    m_logger->log(QStringLiteral("定时认证触发（间隔%1h 抖动%2min）")
                        .arg(m_settings->schedInterval())
                        .arg(m_scheduler->jitterSec() / 60));

                    QMetaObject::invokeMethod(m_tray, [this]() {
                        m_tray->updateIcon(Platform::SystemTray::Status::Authenticating);
                    }, Qt::QueuedConnection);

                    QString mac = Platform::NetworkAdapter::getMacAddress();
                    QString ip  = Platform::NetworkAdapter::getLocalIp(
                        QStringLiteral("172.30.255.2"));
                    if (!ip.isEmpty()) {
                        m_authEngine->authenticate(mac, ip);
                    }
                }
                m_scheduler->recordTrigger(nowSec);
            }
        }

        // === 连通性检测 ===
        bool connected = m_connectivity->check();
        if (connected) {
            if (fails > 0) {
                m_logger->log(QStringLiteral("网络已恢复"));
            }
            QMetaObject::invokeMethod(m_tray, [this]() {
                m_tray->updateIcon(Platform::SystemTray::Status::Connected);
            }, Qt::QueuedConnection);
            QMetaObject::invokeMethod(m_trayVM, [this]() {
                m_trayVM->onAuthStateChanged(0); // Connected
            }, Qt::QueuedConnection);
            fails = 0;
            prevAirplane = false;
            interruptibleSleep(30);
            continue;
        }

        // === 飞行模式检测 ===
        if (Platform::NetworkAdapter::isAirplaneMode()) {
            if (!prevAirplane) {
                m_logger->log(QStringLiteral("飞行模式已开启，跳过认证"));
                prevAirplane = true;
            }
            interruptibleSleep(60);
            continue;
        }
        prevAirplane = false;

        // === 黑名单时段检测 ===
        if (m_authEngine->isInBlackout()) {
            m_logger->log(QStringLiteral("禁止认证时段，等待 5min"));
            interruptibleSleep(300);
            continue;
        }

        // === 认证尝试 ===
        fails++;
        QString mac = Platform::NetworkAdapter::getMacAddress();
        QString ip  = Platform::NetworkAdapter::getLocalIp(
            QStringLiteral("172.30.255.2"));

        if (ip.isEmpty()) {
            if (!Platform::NetworkAdapter::hasWlanAdapter()) {
                m_logger->log(QStringLiteral("无 WLAN 网卡，等待 60s"));
                interruptibleSleep(60);
            } else {
                int wait = m_authEngine->getExponentialBackoff(fails);
                m_logger->log(QStringLiteral("无IP，等待 %1s (%2)").arg(wait).arg(fails));
                interruptibleSleep(wait);
            }
            continue;
        }

        m_logger->log(QStringLiteral("网络断开，尝试登录 (%1)").arg(fails));

        QMetaObject::invokeMethod(m_tray, [this]() {
            m_tray->updateIcon(Platform::SystemTray::Status::Authenticating);
        }, Qt::QueuedConnection);

        AuthEngine::AuthResult result = m_authEngine->authenticate(mac, ip);

        if (result == AuthEngine::AuthResult::Success
            || result == AuthEngine::AuthResult::AlreadyOnline) {
            fails = 0;

            QMetaObject::invokeMethod(m_tray, [this]() {
                m_tray->updateIcon(Platform::SystemTray::Status::Connected);
            }, Qt::QueuedConnection);

            // 自动开热点
            if (m_settings->autoHotspot() && m_hotspot) {
                bool ok = m_hotspot->start();
                m_logger->log(QStringLiteral("自动热点: %1").arg(ok ? QStringLiteral("已开启") : QStringLiteral("开启失败")));
            }

            // 成功后短等待
            interruptibleSleep(10);
        } else {
            int backoff = m_authEngine->getExponentialBackoff(fails);
            m_logger->log(QStringLiteral("登录失败，等待 %1s (%2)").arg(backoff).arg(fails));

            QMetaObject::invokeMethod(m_tray, [this]() {
                m_tray->updateIcon(Platform::SystemTray::Status::Disconnected);
            }, Qt::QueuedConnection);

            interruptibleSleep(backoff);
        }
    }

    m_logger->log(QStringLiteral("工作线程退出"));
}

void App::interruptibleSleep(int seconds)
{
    for (int i = 0; i < seconds && m_running.load(); ++i) {
        QThread::sleep(1);
    }
}

#ifdef _WIN32
// DWM 圆角偏好常量（MinGW dwmapi.h 中可能不存在，手动定义）
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif

// 动态加载 dwmapi.dll 并调用 DwmSetWindowAttribute（避免 MinGW 链接问题）
static bool applyDwmCornerPreference(HWND hwnd)
{
    // 静态函数指针：初始化一次（用 IIFE 避免捕获问题）
    static bool (WINAPI *pfn)(HWND, DWORD, LPCVOID, DWORD) = []() -> decltype(pfn) {
        HMODULE mod = LoadLibraryW(L"dwmapi.dll");
        if (!mod) return nullptr;
        return reinterpret_cast<decltype(pfn)>(
            reinterpret_cast<void*>(GetProcAddress(mod, "DwmSetWindowAttribute")));
    }();

    if (!pfn) {
        qWarning() << "DWM: 无法解析 DwmSetWindowAttribute（dwmapi.dll 加载失败）";
        return false;
    }

    DWORD preference = DWMWCP_ROUND;
    HRESULT hr = pfn(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
    if (FAILED(hr))
        qWarning() << "DWM: DwmSetWindowAttribute 失败 hr=" << hr;
    return SUCCEEDED(hr);
}

void App::applyWindowMask(QWindow *win, int radius)
{
    Q_UNUSED(radius);
    if (!win) return;
    HWND hwnd = reinterpret_cast<HWND>(win->winId());
    if (!hwnd) return;

    // 使用 DWM API 设置圆角（与 SetWindowRgn 不同，不会与 DWM 冲突）
    // 注意：DWM 圆角由系统固定，不可自定义半径
    applyDwmCornerPreference(hwnd);
}
#else
void App::applyWindowMask(QWindow *win, int radius)
{
    Q_UNUSED(win);
    Q_UNUSED(radius);
}
#endif

void App::applyMasksToAllWindows()
{
    for (QWindow *win : qApp->allWindows()) {
        applyWindowMask(win, 24);
    }
}

