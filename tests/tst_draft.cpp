#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "core/draft_store.h"

class TestDraft : public QObject
{
    Q_OBJECT

private slots:
    void missingFile();
    void saveAndLoad();
    void saveEmptyAndLoadEmpty();
    void saveKeepsLfOnly();
    void loadNormalizesNewlines();
    void loadStripsBom();
    void loadRejectsInvalidUtf8();
    void removeDeletesFile();
    void saveReportsFailure();
};

void TestDraft::missingFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));

    QString text;
    QCOMPARE(store.load(&text), DraftStore::LoadResult::Missing);
}

void TestDraft::saveAndLoad()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));

    const QString content = QStringLiteral(u"こんにちは\n世界");
    QString error;
    QVERIFY2(store.save(content, &error), qPrintable(error));

    QString text;
    QCOMPARE(store.load(&text), DraftStore::LoadResult::Ok);
    QCOMPARE(text, content);
}

void TestDraft::saveEmptyAndLoadEmpty()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));

    QVERIFY(store.save(QString()));
    QVERIFY(QFile::exists(store.path()));

    QString text;
    QCOMPARE(store.load(&text), DraftStore::LoadResult::Empty);
}

void TestDraft::saveKeepsLfOnly()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));

    QVERIFY(store.save(QStringLiteral(u"a\nb")));

    QFile file(store.path());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("a\nb"));
}

void TestDraft::loadNormalizesNewlines()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));

    QFile file(store.path());
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray("a\r\nb\rc\nd"));
    file.close();

    QString text;
    QCOMPARE(store.load(&text), DraftStore::LoadResult::Ok);
    QCOMPARE(text, QStringLiteral(u"a\nb\nc\nd"));
}

void TestDraft::loadStripsBom()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));

    QFile file(store.path());
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray("\xEF\xBB\xBF") + QStringLiteral(u"あ").toUtf8());
    file.close();

    QString text;
    QCOMPARE(store.load(&text), DraftStore::LoadResult::Ok);
    QCOMPARE(text, QStringLiteral(u"あ"));
}

void TestDraft::loadRejectsInvalidUtf8()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));

    QFile file(store.path());
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray("\xFF\xFE\x61\x62"));
    file.close();

    QString text;
    QString error;
    QCOMPARE(store.load(&text, &error), DraftStore::LoadResult::Error);
    QVERIFY(!error.isEmpty());
    QVERIFY(QFile::exists(store.path()));
}

void TestDraft::removeDeletesFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    DraftStore store(dir.filePath(QStringLiteral("draft.txt")));

    QVERIFY(store.save(QStringLiteral(u"あ")));
    QVERIFY(QFile::exists(store.path()));
    store.remove();
    QVERIFY(!QFile::exists(store.path()));

    QString text;
    QCOMPARE(store.load(&text), DraftStore::LoadResult::Missing);
}

void TestDraft::saveReportsFailure()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString blockerPath = dir.filePath(QStringLiteral("blocker"));
    QFile blocker(blockerPath);
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.write("x");
    blocker.close();

    DraftStore store(blockerPath + QStringLiteral("/draft.txt"));
    QString error;
    QVERIFY(!store.save(QStringLiteral(u"あ"), &error));
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(TestDraft)

#include "tst_draft.moc"
