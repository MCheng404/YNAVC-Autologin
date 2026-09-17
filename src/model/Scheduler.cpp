#include "Scheduler.h"
#include "Settings.h"

#include <QRandomGenerator>

Scheduler::Scheduler(Settings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    // 每个周期开始摇一次抖动（±5 分钟）
    m_jitterSec = QRandomGenerator::global()->bounded(-300, 301);
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

    qint64 base = 0;
    if (m_lastSchedAuth == 0) {
        // 首次：从当前时间往前推到最近的 interval 边界
        qint64 nowSec = QDateTime::currentSecsSinceEpoch();
        if (intervalSec > 0) {
            qint64 elapsed = (nowSec % intervalSec);
            base = nowSec - elapsed;
        } else {
            base = nowSec;
        }
    } else {
        base = m_lastSchedAuth;
    }

    // 抖动在每个周期只摇一次（构造函数/recordTrigger），此处直接叠加
    qint64 next = base + intervalSec + m_jitterSec;

    return next;
}
