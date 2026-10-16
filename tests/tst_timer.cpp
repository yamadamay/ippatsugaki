#include <QtTest>

#include <QSignalSpy>

#include "core/session_timer.h"
#include "fake_clock.h"

class TestTimer : public QObject
{
    Q_OBJECT

private slots:
    void countsElapsedTime();
    void delayedTickDoesNotAccumulate();
    void pauseFreezesRemainingAndResumeContinues();
    void sleepDetectedWhenDeltaExceedsThreshold();
    void deltaOfExactlyThresholdIsCounted();
    void expiryEmitsOnceAndStops();
};

void TestTimer::countsElapsedTime()
{
    FakeClock clock;
    SessionTimer timer(&clock);
    timer.start(60000);
    QCOMPARE(timer.remainingMs(), qint64(60000));
    QVERIFY(timer.isRunning());

    clock.advance(200);
    timer.onTick();
    QCOMPARE(timer.remainingMs(), qint64(59800));

    clock.advance(200);
    timer.onTick();
    QCOMPARE(timer.remainingMs(), qint64(59600));
}

void TestTimer::delayedTickDoesNotAccumulate()
{
    FakeClock clock;
    SessionTimer timer(&clock);
    QSignalSpy tickSpy(&timer, &SessionTimer::tick);
    QSignalSpy sleepSpy(&timer, &SessionTimer::sleepDetected);

    timer.start(60000);
    clock.advance(2000);
    timer.onTick();

    QCOMPARE(timer.remainingMs(), qint64(58000));
    QCOMPARE(sleepSpy.count(), 0);
    QVERIFY(tickSpy.count() >= 2);
}

void TestTimer::pauseFreezesRemainingAndResumeContinues()
{
    FakeClock clock;
    SessionTimer timer(&clock);

    timer.start(60000);
    clock.advance(1000);
    timer.onTick();
    QCOMPARE(timer.remainingMs(), qint64(59000));

    timer.pause();
    QVERIFY(!timer.isRunning());
    clock.advance(5000);
    timer.onTick();
    QCOMPARE(timer.remainingMs(), qint64(59000));

    timer.resume();
    QVERIFY(timer.isRunning());
    clock.advance(1000);
    timer.onTick();
    QCOMPARE(timer.remainingMs(), qint64(58000));
}

void TestTimer::sleepDetectedWhenDeltaExceedsThreshold()
{
    FakeClock clock;
    SessionTimer timer(&clock);
    QSignalSpy sleepSpy(&timer, &SessionTimer::sleepDetected);

    timer.start(60000);
    clock.advance(SessionTimer::sleepThresholdMs + 1);
    timer.onTick();

    QCOMPARE(sleepSpy.count(), 1);
    QCOMPARE(timer.remainingMs(), qint64(60000));

    clock.advance(200);
    timer.onTick();
    QCOMPARE(timer.remainingMs(), qint64(59800));
}

void TestTimer::deltaOfExactlyThresholdIsCounted()
{
    FakeClock clock;
    SessionTimer timer(&clock);
    QSignalSpy sleepSpy(&timer, &SessionTimer::sleepDetected);

    timer.start(60000);
    clock.advance(SessionTimer::sleepThresholdMs);
    timer.onTick();

    QCOMPARE(sleepSpy.count(), 0);
    QCOMPARE(timer.remainingMs(), qint64(60000 - SessionTimer::sleepThresholdMs));
}

void TestTimer::expiryEmitsOnceAndStops()
{
    FakeClock clock;
    SessionTimer timer(&clock);
    QSignalSpy expiredSpy(&timer, &SessionTimer::expired);

    timer.start(1000);
    clock.advance(1000);
    timer.onTick();

    QCOMPARE(expiredSpy.count(), 1);
    QCOMPARE(timer.remainingMs(), qint64(0));
    QVERIFY(!timer.isRunning());

    clock.advance(200);
    timer.onTick();
    QCOMPARE(expiredSpy.count(), 1);
}

QTEST_GUILESS_MAIN(TestTimer)

#include "tst_timer.moc"
