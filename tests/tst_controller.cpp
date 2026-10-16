#include <QtTest>

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "core/draft_store.h"
#include "core/session_controller.h"
#include "fake_clock.h"

class TestController : public QObject
{
    Q_OBJECT

private slots:
    void initialState();
    void confirmStartsWritingAndCreatesDraft();
    void noRedrawAfterConfirm();
    void redrawAvoidsImmediateRepeat();
    void documentChangesIgnoredOutsideWriting();
    void pauseSavesDraftAndFreezesTimer();
    void sleepAutoPauses();
    void timeoutEntersSavingWithDraft();
    void giveUpWithEmptyTextReturnsToStart();
    void giveUpWithTextEntersSaving();
    void finishSaveSuccessCleansUp();
    void finishSaveDiscardedCleansUp();
    void autosaveWritesChangedText();
    void timeLimitSelection();
    void recoveryRecoverable();
    void recoveryEmptyDraftIsDeleted();
    void recoveryUnreadableKeepsFile();

private:
    static PromptData testData();
    static QByteArray readFile(const QString &path);
};

PromptData TestController::testData()
{
    return PromptData{{QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C")},
                      {QStringLiteral("E1"), QStringLiteral("E2")},
                      {QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")}};
}

QByteArray TestController::readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
    return file.readAll();
}

void TestController::initialState()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);

    controller.startWithData(testData());

    QCOMPARE(controller.state(), SessionController::State::ChoosePrompt);
    const Prompt prompt = controller.currentPrompt();
    QVERIFY(!prompt.subject.isEmpty());
    QVERIFY(!prompt.event.isEmpty());
    QVERIFY(!prompt.element.isEmpty());
    QCOMPARE(controller.remainingMs(), qint64(30) * 60000);
}

void TestController::confirmStartsWritingAndCreatesDraft()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    QVERIFY(controller.confirmStart());
    QCOMPARE(controller.state(), SessionController::State::Writing);
    QVERIFY(QFile::exists(store.path()));
    QCOMPARE(controller.remainingMs(), qint64(30) * 60000);
    QVERIFY(!controller.confirmStart());
}

void TestController::noRedrawAfterConfirm()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    controller.confirmStart();
    const Prompt before = controller.currentPrompt();
    controller.redrawPrompt();
    const Prompt after = controller.currentPrompt();

    QCOMPARE(after.subject, before.subject);
    QCOMPARE(after.event, before.event);
    QCOMPARE(after.element, before.element);
}

void TestController::redrawAvoidsImmediateRepeat()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    const Prompt first = controller.currentPrompt();
    controller.redrawPrompt();
    const Prompt second = controller.currentPrompt();

    QVERIFY(second.subject != first.subject);
    QVERIFY(second.event != first.event);
    QVERIFY(second.element != first.element);
}

void TestController::documentChangesIgnoredOutsideWriting()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    controller.noteDocumentChanged(QStringLiteral(u"無視される"));
    QCOMPARE(controller.documentText(), QString());
}

void TestController::pauseSavesDraftAndFreezesTimer()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    controller.confirmStart();
    controller.noteDocumentChanged(QStringLiteral(u"途中の文章"));
    controller.pause();

    QCOMPARE(controller.state(), SessionController::State::Paused);
    QCOMPARE(readFile(store.path()), QStringLiteral(u"途中の文章").toUtf8());

    clock.advance(5000);
    controller.handleTick();
    QCOMPARE(controller.remainingMs(), qint64(30) * 60000);

    controller.resume();
    QCOMPARE(controller.state(), SessionController::State::Writing);
    clock.advance(1000);
    controller.handleTick();
    QCOMPARE(controller.remainingMs(), qint64(30) * 60000 - 1000);
}

void TestController::sleepAutoPauses()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    controller.confirmStart();
    QSignalSpy spy(&controller, &SessionController::sleepAutoPaused);

    clock.advance(SessionTimer::sleepThresholdMs + 1);
    controller.handleTick();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(controller.state(), SessionController::State::Paused);
    QCOMPARE(controller.remainingMs(), qint64(30) * 60000);
}

void TestController::timeoutEntersSavingWithDraft()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    controller.setTimeLimitMinutes(10);
    controller.confirmStart();
    controller.noteDocumentChanged(QStringLiteral(u"本文"));

    QSignalSpy spy(&controller, &SessionController::timedOut);
    for (int elapsed = 0; elapsed < 600000; elapsed += SessionTimer::tickIntervalMs) {
        clock.advance(SessionTimer::tickIntervalMs);
        controller.handleTick();
    }

    QCOMPARE(spy.count(), 1);
    QCOMPARE(controller.state(), SessionController::State::Saving);
    QCOMPARE(controller.remainingMs(), qint64(0));
    QCOMPARE(readFile(store.path()), QStringLiteral(u"本文").toUtf8());
}

void TestController::giveUpWithEmptyTextReturnsToStart()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    controller.confirmStart();
    QVERIFY(QFile::exists(store.path()));

    controller.giveUp();

    QCOMPARE(controller.state(), SessionController::State::ChoosePrompt);
    QVERIFY(!QFile::exists(store.path()));
}

void TestController::giveUpWithTextEntersSaving()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    controller.confirmStart();
    controller.noteDocumentChanged(QStringLiteral(u"書きかけ"));
    controller.giveUp();

    QCOMPARE(controller.state(), SessionController::State::Saving);
    QCOMPARE(readFile(store.path()), QStringLiteral(u"書きかけ").toUtf8());
}

void TestController::finishSaveSuccessCleansUp()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    controller.confirmStart();
    controller.noteDocumentChanged(QStringLiteral(u"書きかけ"));
    controller.giveUp();
    const Prompt before = controller.currentPrompt();

    controller.finishSaveSuccess();

    QCOMPARE(controller.state(), SessionController::State::ChoosePrompt);
    QVERIFY(!QFile::exists(store.path()));
    QCOMPARE(controller.documentText(), QString());
    QVERIFY(controller.currentPrompt().subject != before.subject);
    QCOMPARE(controller.remainingMs(), qint64(30) * 60000);
}

void TestController::finishSaveDiscardedCleansUp()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    controller.confirmStart();
    controller.noteDocumentChanged(QStringLiteral(u"書きかけ"));
    controller.giveUp();

    controller.finishSaveDiscarded();

    QCOMPARE(controller.state(), SessionController::State::ChoosePrompt);
    QVERIFY(!QFile::exists(store.path()));
    QCOMPARE(controller.documentText(), QString());
}

void TestController::autosaveWritesChangedText()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    controller.confirmStart();
    controller.noteDocumentChanged(QStringLiteral(u"z"));
    controller.autosaveTick();
    QCOMPARE(readFile(store.path()), QByteArray("z"));

    controller.autosaveTick();
    QCOMPARE(readFile(store.path()), QByteArray("z"));

    controller.noteDocumentChanged(QStringLiteral(u"zz"));
    controller.autosaveTick();
    QCOMPARE(readFile(store.path()), QByteArray("zz"));
}

void TestController::timeLimitSelection()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    QCOMPARE(controller.timeLimitMinutes(), 30);
    controller.setTimeLimitMinutes(10);
    QCOMPARE(controller.remainingMs(), qint64(10) * 60000);

    controller.confirmStart();
    controller.setTimeLimitMinutes(45);
    QCOMPARE(controller.timeLimitMinutes(), 10);
    QCOMPARE(controller.remainingMs(), qint64(10) * 60000);
}

void TestController::recoveryRecoverable()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    QVERIFY(store.save(QStringLiteral(u"前回の文章")));

    QString text;
    QCOMPARE(controller.checkDraft(&text), SessionController::RecoveryCheck::Recoverable);
    QCOMPARE(text, QStringLiteral(u"前回の文章"));

    controller.recoverInto(text);
    QCOMPARE(controller.state(), SessionController::State::Saving);
    QCOMPARE(controller.documentText(), text);
}

void TestController::recoveryEmptyDraftIsDeleted()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    QString text;
    QCOMPARE(controller.checkDraft(&text), SessionController::RecoveryCheck::Nothing);

    QVERIFY(store.save(QString()));
    QCOMPARE(controller.checkDraft(&text), SessionController::RecoveryCheck::Nothing);
    QVERIFY(!QFile::exists(store.path()));
}

void TestController::recoveryUnreadableKeepsFile()
{
    FakeClock clock;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));
    SessionController controller(&clock, &store);
    controller.startWithData(testData());

    QFile file(store.path());
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray("\xFF\xFE\x61"));
    file.close();

    QString text;
    QCOMPARE(controller.checkDraft(&text), SessionController::RecoveryCheck::Unreadable);
    QVERIFY(QFile::exists(store.path()));
}

QTEST_GUILESS_MAIN(TestController)

#include "tst_controller.moc"
