#pragma once

#include <QElapsedTimer>
#include <QtGlobal>

class Clock
{
public:
    virtual ~Clock() = default;
    virtual qint64 nowMs() const = 0;
};

class SystemClock : public Clock
{
public:
    SystemClock();
    qint64 nowMs() const override;

private:
    QElapsedTimer m_elapsed;
};
