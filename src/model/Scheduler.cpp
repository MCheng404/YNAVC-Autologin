#include "Scheduler.h"
#include "Settings.h"

#include <QRandomGenerator>
#include <QDebug>

Scheduler::Scheduler(Settings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    // 每个周期开始摇一次抖动（±5 分钟）
    m_jitterSec = QRandomGenerator::global()->bounded(-300, 301);

    // 设置变更（启停 / 间隔）时，以当前时刻为基准重置调度周期
    connect(m_settings, &Settings::schedEnabledChanged,  this, &Scheduler::resetSchedule);
    connect(m_settings, &Settings::schedIntervalChanged, this, &Scheduler::resetSchedule);
}

void Scheduler::resetSchedule()
{
    const qint64 nowSec = QDateTime::currentSecsSinceEpoch();
    m_lastSchedAuth = nowSec;
    m_jitterSec = QRandomGenerator::global()->bounded(-300, 301);
    // 便于在 debug.log 里确认设置确实生效了
    qInfo() << "[Scheduler] 调度已重置：基准 =" << nowSec
            << " 间隔 =" << m_settings->schedInterval() << "h"
            << " 抖动 =" << (m_jitterSec.load() / 60) << "min"
            << " 启用 =" << m_settings->schedEnabled();
}

bool Scheduler::shouldTrigger(qint64 nowSec)
{
    if (!m_settings->schedEnabled()) return false;

    qint64 next = nextTriggerTime();
    return (nowSec >= next);
}

void Scheduler::recordTrigger(qint64 ts)
{
    m_lastSchedAuth = ts;
    // 进入下一周期，重新摇一次抖动（±5 分钟，每个周期只摇一次）
    m_jitterSec = QRandomGenerator::global()->bounded(-300, 301);
}

qint64 Scheduler::nextTriggerTime() const
{
    qint64 intervalSec = static_cast<qint64>(m_settings->schedInterval()) * 3600;

    // 原子量各读一次到局部变量，避免同一表达式内多次读取导致 base 与 next 取值不一致
    const qint64 lastAuth = m_lastSchedAuth.load();
    const qint64 jitter   = m_jitterSec.load();

    qint64 base = 0;
    if (lastAuth == 0) {
        // 首次：从当前时间往前推到最近的 interval 边界
        qint64 nowSec = QDateTime::currentSecsSinceEpoch();
        if (intervalSec > 0) {
            qint64 elapsed = (nowSec % intervalSec);
            base = nowSec - elapsed;
        } else {
            base = nowSec;
        }
    } else {
        base = lastAuth;
    }

    // 抖动在每个周期只摇一次（构造函数/recordTrigger），此处直接叠加
    qint64 next = base + intervalSec + jitter;

    return next;
}
