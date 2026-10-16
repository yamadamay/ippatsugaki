#include "clock.h"

SystemClock::SystemClock()
{
    m_elapsed.start();
}

qint64 SystemClock::nowMs() const
{
    return m_elapsed.elapsed();
}
