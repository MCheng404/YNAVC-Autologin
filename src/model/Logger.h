#pragma once

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QString>
#include <QDate>

/**
 * @brief 线程安全文件日志 + 7天自动清理
 *
 * 日志目录：%APPDATA%/AutoLogin/logs/
 * 文件名格式：autologin_YYYYMMDD.log
 * 7 天自动清理，支持手动清理按钮
 */
class Logger : public QObject {
    Q_OBJECT

    /**
     * 最近日志（内存缓冲，时间正序：最旧在前、最新在后）。
     * 供 QML（LogCard）绑定显示；变化时 emit recentLogsChanged()。
     */
    Q_PROPERTY(QStringList recentLogs READ recentLogs NOTIFY recentLogsChanged)

public:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();

    /** 写入日志消息（线程安全） */
    void log(const QString &msg);

    /** 自动清理超过 7 天的日志文件 */
    void autoCleanup();

    /** 手动清理所有日志文件，返回 true 表示清理成功 */
    bool manualCleanup();

    /** 获取日志目录路径 */
    QString logPath() const;

    /** 最近日志列表（时间正序），线程安全：持锁短暂拷贝 */
    QStringList recentLogs() const;

signals:
    /** 新日志消息信号 */
    void logMessage(const QString &msg);

    /** 最近日志缓冲发生变化（供 QML 绑定刷新） */
    void recentLogsChanged();

private:
    /** 确保日志目录存在并打开当天的日志文件 */
    void ensureLogFile();

    /** 保留的最近日志条数上限（环形缓冲） */
    static constexpr int kMaxRecent = 200;

    QFile       m_logFile;
    QString     m_logPath;
    QDate       m_currentDate;
    mutable QMutex m_mutex;          // mutable：供 const 的 recentLogs() 持锁拷贝

    /** 最近日志定长环形缓冲（时间正序，每条含时间戳，可直接显示） */
    QStringList m_recent;
};
