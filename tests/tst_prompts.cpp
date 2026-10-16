#include <QtTest>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "core/prompt_source.h"
#include "core/text_utils.h"

class TestPrompts : public QObject
{
    Q_OBJECT

private slots:
    void drawAvoidsConsecutiveRepeat();
    void drawAllowsRepeatWithSingleCandidate();
    void missingFileFallsBackToEmbedded();
    void invalidJsonFallsBackToEmbedded();
    void invalidEntriesAreFiltered();
    void emptyListIsFilledFromEmbedded();
    void embeddedDataMeetsMinimum();
    void shippedDataFileIsValid();

private:
    static QString writeTempFile(QTemporaryDir &dir, const QByteArray &content);
};

QString TestPrompts::writeTempFile(QTemporaryDir &dir, const QByteArray &content)
{
    const QString path = dir.filePath(QStringLiteral("prompts.json"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return QString();
    file.write(content);
    file.close();
    return path;
}

void TestPrompts::drawAvoidsConsecutiveRepeat()
{
    const PromptData data{{QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C")},
                          {QStringLiteral("E1"), QStringLiteral("E2")},
                          {QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")}};
    PromptSource source(data);

    QString lastSubject;
    QString lastEvent;
    QString lastElement;
    for (int i = 0; i < 200; ++i) {
        const Prompt prompt = source.draw();
        QVERIFY(data.subjects.contains(prompt.subject));
        QVERIFY(data.events.contains(prompt.event));
        QVERIFY(data.elements.contains(prompt.element));
        if (i > 0) {
            QVERIFY(prompt.subject != lastSubject);
            QVERIFY(prompt.event != lastEvent);
            QVERIFY(prompt.element != lastElement);
        }
        lastSubject = prompt.subject;
        lastEvent = prompt.event;
        lastElement = prompt.element;
    }
}

void TestPrompts::drawAllowsRepeatWithSingleCandidate()
{
    const PromptData data{{QStringLiteral("A")}, {QStringLiteral("E")}, {QStringLiteral("X")}};
    PromptSource source(data);

    const Prompt first = source.draw();
    const Prompt second = source.draw();
    QCOMPARE(first.subject, QStringLiteral("A"));
    QCOMPARE(second.subject, QStringLiteral("A"));
    QCOMPARE(second.event, QStringLiteral("E"));
    QCOMPARE(second.element, QStringLiteral("X"));
}

void TestPrompts::missingFileFallsBackToEmbedded()
{
    bool usedEmbedded = false;
    QString message;
    const PromptData data = PromptSource::loadFromFile(
        QStringLiteral("/nonexistent/directory/prompts.json"), &usedEmbedded, &message);
    QVERIFY(usedEmbedded);
    QVERIFY(!message.isEmpty());
    QVERIFY(data.subjects.size() >= 20);
    QVERIFY(data.events.size() >= 20);
    QVERIFY(data.elements.size() >= 20);
}

void TestPrompts::invalidJsonFallsBackToEmbedded()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = writeTempFile(dir, QByteArray("{ this is not json"));
    QVERIFY(!path.isEmpty());

    bool usedEmbedded = false;
    QString message;
    const PromptData data = PromptSource::loadFromFile(path, &usedEmbedded, &message);
    QVERIFY(usedEmbedded);
    QVERIFY(!message.isEmpty());
    QVERIFY(data.subjects.size() >= 20);
}

void TestPrompts::invalidEntriesAreFiltered()
{
    QJsonObject obj;
    obj.insert(QStringLiteral("subjects"),
               QJsonArray{QStringLiteral("OK"), 42, QStringLiteral(""), QStringLiteral("   "),
                          QString(120, QLatin1Char('a')), QStringLiteral("OK2")});
    obj.insert(QStringLiteral("events"), QJsonArray{QStringLiteral("E1")});
    obj.insert(QStringLiteral("elements"), QJsonArray{QStringLiteral("X1")});

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = writeTempFile(dir, QJsonDocument(obj).toJson());
    QVERIFY(!path.isEmpty());

    bool usedEmbedded = true;
    QString message;
    const PromptData data = PromptSource::loadFromFile(path, &usedEmbedded, &message);
    QVERIFY(!usedEmbedded);
    QVERIFY(message.isEmpty());
    QCOMPARE(data.subjects, QStringList({QStringLiteral("OK"), QStringLiteral("OK2")}));
    QCOMPARE(data.events, QStringList({QStringLiteral("E1")}));
    QCOMPARE(data.elements, QStringList({QStringLiteral("X1")}));
}

void TestPrompts::emptyListIsFilledFromEmbedded()
{
    QJsonObject obj;
    obj.insert(QStringLiteral("subjects"), QJsonArray{});
    obj.insert(QStringLiteral("events"), QJsonArray{QStringLiteral("E1")});
    obj.insert(QStringLiteral("elements"), QJsonArray{QStringLiteral("X1")});

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = writeTempFile(dir, QJsonDocument(obj).toJson());
    QVERIFY(!path.isEmpty());

    bool usedEmbedded = false;
    QString message;
    const PromptData data = PromptSource::loadFromFile(path, &usedEmbedded, &message);
    QVERIFY(usedEmbedded);
    QVERIFY(!message.isEmpty());
    QVERIFY(data.subjects.size() >= 20);
    QCOMPARE(data.events, QStringList({QStringLiteral("E1")}));
    QCOMPARE(data.elements, QStringList({QStringLiteral("X1")}));
}

void TestPrompts::embeddedDataMeetsMinimum()
{
    const PromptData data = PromptSource::embeddedData();
    QVERIFY(data.subjects.size() >= 20);
    QVERIFY(data.events.size() >= 20);
    QVERIFY(data.elements.size() >= 20);

    const QList<QStringList> lists = {data.subjects, data.events, data.elements};
    for (const QStringList &list : lists) {
        for (const QString &entry : list) {
            QVERIFY(!entry.trimmed().isEmpty());
            QVERIFY(textutil::countCodePoints(entry) <= PromptSource::maxEntryCodePoints);
        }
    }
}

void TestPrompts::shippedDataFileIsValid()
{
    bool usedEmbedded = true;
    QString message;
    const PromptData data = PromptSource::loadFromFile(QStringLiteral(PROMPTS_JSON_PATH),
                                                       &usedEmbedded, &message);
    QVERIFY2(!usedEmbedded, qPrintable(message));
    QVERIFY(data.subjects.size() >= 100);
    QVERIFY(data.events.size() >= 100);
    QVERIFY(data.elements.size() >= 100);

    const QList<QStringList> lists = {data.subjects, data.events, data.elements};
    for (const QStringList &list : lists) {
        QStringList unique = list;
        unique.removeDuplicates();
        QCOMPARE(unique.size(), list.size());
        for (const QString &entry : list) {
            QVERIFY(!entry.trimmed().isEmpty());
            QVERIFY(textutil::countCodePoints(entry) <= PromptSource::maxEntryCodePoints);
        }
    }
}

QTEST_APPLESS_MAIN(TestPrompts)

#include "tst_prompts.moc"
