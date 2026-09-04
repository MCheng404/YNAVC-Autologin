#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QFile>
#include <QDateTime>

#ifdef Q_OS_WIN
// Winsock2 必须在 windows.h 之前包含（避免 fd_set 重定义）
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "App.h"

// 调试日志文件（临时诊断用）
static QFile g_debugLog;

static void debugMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // 写入文件
    if (g_debugLog.isOpen()) {
        QString ts = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
        g_debugLog.write(QString("[%1] %2\n").arg(ts, msg).toUtf8());
        g_debugLog.flush();
    }
    // 也输出到默认 handler（调试器可见）
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // Windows 平台：设置进程 DPI 感知（Per-Monitor V2）
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    // 安装消息处理器，所有 Qt/QML 警告写入日志文件
    g_debugLog.setFileName(QStringLiteral("debug.log"));
    g_debugLog.open(QIODevice::WriteOnly | QIODevice::Append);
    qInstallMessageHandler(debugMessageHandler);

    // 使用 QApplication（而非 QGuiApplication）：SystemTray 的 QMenu 是 Widgets 类
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("AutoLogin"));
    app.setApplicationName(QStringLiteral("校园网自动登录"));
    app.setApplicationVersion(QStringLiteral("2.13"));

    // 关键：关闭窗口不退出应用（托盘常驻）
    app.setQuitOnLastWindowClosed(false);

#ifdef Q_OS_WIN
    // Winsock 初始化（NetworkAdapter::getLocalIp 需要）
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    // 创建应用编排器
    App autologin;

    if (!autologin.initialize()) {
#ifdef Q_OS_WIN
        WSACleanup();
#endif
        return -1;
    }

    int ret = autologin.run();

#ifdef Q_OS_WIN
    WSACleanup();
#endif

    g_debugLog.close();
    return ret;
}
