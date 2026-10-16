#include "session_timer.h"

SessionTimer::SessionTimer(Clock *clock, QObject *parent)
    : QObject(parent)
    , m_clock(clock)
{
    m_timer.setInterval(tickIntervalMs);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &SessionTimer::onTick);
}

void SessionTimer::start(qint64 limitMs)
{
    m_remainingMs = limitMs;
    m_lastTickMs = m_clock->nowMs();
    m_running = true;
    m_timer.start();
    emit runningChanged(true);
    emit tick(m_remainingMs);
}

void SessionTimer::pause()
{
    if (!m_running)
        return;
    m_running = false;
    m_timer.stop();
    emit runningChanged(false);
    emit tick(m_remainingMs);
}

void SessionTimer::resume()
{
    if (m_running)
        return;
    m_lastTickMs = m_clock->nowMs();
    m_running = true;
    m_timer.start();
    emit runningChanged(true);
    emit tick(m_remainingMs);
}

void SessionTimer::stop()
{
    if (!m_running) {
        m_timer.stop();
        return;
    }
    m_running = false;
    m_timer.stop();
    emit runningChanged(false);
}

qint64 SessionTimer::remainingMs() const
{
    return m_remainingMs;
}

bool SessionTimer::isRunning() const
{
    return m_running;
}

void SessionTimer::onTick()
{
    if (!m_running)
        return;

    const qint64 now = m_clock->nowMs();
    const qint64 delta = now - m_lastTickMs;
    m_lastTickMs = now;

    if (delta > sleepThresholdMs) {
        emit sleepDetected();
        return;
    }

    m_remainingMs -= delta;
    if (m_remainingMs <= 0) {
        m_remainingMs = 0;
        m_running = false;
        m_timer.stop();
        emit tick(0);
        emit runningChanged(false);
        emit expired();
        return;
    }

    emit tick(m_remainingMs);
}
