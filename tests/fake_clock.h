#pragma once

#include "core/clock.h"

class FakeClock : public Clock
{
public:
    qint64 nowMs() const override { return m_now; }
    void advance(qint64 ms) { m_now += ms; }
    void set(qint64 ms) { m_now = ms; }

private:
    qint64 m_now = 0;
};
