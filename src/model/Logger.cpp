#include "Logger.h"

#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QMutexLocker>

Logger::Logger(QObject *parent)
    : QObject(parent)
{
    m_logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                + QStringLiteral("/logs/");
    ensureLogFile();
    autoCleanup();
}

Logger::~Logger()
{
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

void Logger::ensureLogFile()
{
    QDir dir(m_logPath);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    QDate today = QDate::currentDate();
    QString fileName = QStringLiteral("autologin_%1.log")
        .arg(today.toString(QStringLiteral("yyyyMMdd")));

    QString filePath = dir.filePath(fileName);

    // 如果日期变了或文件未打开，重新打开
    if (!m_logFile.isOpen() || m_currentDate != today) {
        if (m_logFile.isOpen()) {
            m_logFile.close();
        }
        m_logFile.setFileName(filePath);
        m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        m_currentDate = today;
    }
}

void Logger::log(const QString &msg)
{
    QMutexLocker locker(&m_mutex);

    ensureLogFile();

    if (!m_logFile.isWritable()) return;

    QString timestamp = QTime::currentTime().toString(QStringLiteral("[HH:mm:ss] "));
    QString line = timestamp + msg + QLatin1Char('\n');

    QTextStream stream(&m_logFile);
    stream << line;
    stream.flush();

    // 更新内存环形缓冲（时间正序），并通知 QML。
    // 复用同一把锁：日志可能来自 worker 线程，避免引入新的竞态。
    m_recent.append(timestamp + msg);
    if (m_recent.size() > kMaxRecent)
        m_recent.removeFirst();

    emit logMessage(msg);
    emit recentLogsChanged();
}

QStringList Logger::recentLogs() const
{
    // 持锁短暂拷贝后返回，避免把大对象长时间困在临界区内
    QMutexLocker locker(&m_mutex);
    return m_recent;
}

void Logger::autoCleanup()
{
    QDir dir(m_logPath);
    if (!dir.exists()) return;

    QDate cutoff = QDate::currentDate().addDays(-7);

    const QStringList entries = dir.entryList(
        QStringList() << QStringLiteral("autologin_*.log"),
        QDir::Files);

    for (const QString &entry : entries) {
        // 解析日期：autologin_YYYYMMDD.log
        QString dateStr = entry.mid(10, 8); // 跳过 "autologin_" 前缀
        QDate fileDate = QDate::fromString(dateStr, QStringLiteral("yyyyMMdd"));
        if (fileDate.isValid() && fileDate < cutoff) {
            dir.remove(entry);
        }
    }
}

bool Logger::manualCleanup()
{
    QMutexLocker locker(&m_mutex);

    bool success = true;
    QDir dir(m_logPath);
    if (!dir.exists()) return false;

    // 关闭当前日志文件
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }

    const QStringList entries = dir.entryList(
        QStringList() << QStringLiteral("autologin_*.log"),
        QDir::Files);

    for (const QString &entry : entries) {
        if (!dir.remove(entry)) {
            success = false;
        }
    }

    // 重新打开日志文件
    ensureLogFile();

    return success;
}

QString Logger::logPath() const
{
    return m_logPath;
}
