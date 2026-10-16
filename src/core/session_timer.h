#pragma once

#include <QObject>
#include <QTimer>

#include "clock.h"

class SessionTimer : public QObject
{
    Q_OBJECT

public:
    static constexpr int tickIntervalMs = 200;
    static constexpr int sleepThresholdMs = 3000;

    explicit SessionTimer(Clock *clock, QObject *parent = nullptr);

    void start(qint64 limitMs);
    void pause();
    void resume();
    void stop();

    qint64 remainingMs() const;
    bool isRunning() const;

    void onTick();

signals:
    void tick(qint64 remainingMs);
    void expired();
    void sleepDetected();
    void runningChanged(bool running);

private:
    Clock *m_clock = nullptr;
    QTimer m_timer;
    qint64 m_remainingMs = 0;
    qint64 m_lastTickMs = 0;
    bool m_running = false;
};
