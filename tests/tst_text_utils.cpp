#include <QtTest>

#include "core/text_utils.h"

class TestTextUtils : public QObject
{
    Q_OBJECT

private slots:
    void normalizeNewlines();
    void countCodePoints();
    void countVisibleChars();
};

void TestTextUtils::normalizeNewlines()
{
    QCOMPARE(textutil::normalizeNewlines(QStringLiteral(u"a\r\nb\rc\nd")),
             QStringLiteral(u"a\nb\nc\nd"));
    QCOMPARE(textutil::normalizeNewlines(QStringLiteral(u"\r\n")), QStringLiteral(u"\n"));
    QCOMPARE(textutil::normalizeNewlines(QStringLiteral(u"")), QStringLiteral(u""));
    QCOMPARE(textutil::normalizeNewlines(QStringLiteral(u"そのまま")), QStringLiteral(u"そのまま"));
}

void TestTextUtils::countCodePoints()
{
    QCOMPARE(textutil::countCodePoints(QStringLiteral(u"あいう")), 3);
    QCOMPARE(textutil::countCodePoints(QStringLiteral(u"😀")), 1);
    QCOMPARE(textutil::countCodePoints(QStringLiteral(u"😀あ")), 2);
    QCOMPARE(textutil::countCodePoints(QString()), 0);
}

void TestTextUtils::countVisibleChars()
{
    QCOMPARE(textutil::countVisibleChars(QStringLiteral(u"あいう")), 3);
    QCOMPARE(textutil::countVisibleChars(QStringLiteral(u"a b\tc\nd")), 4);
    QCOMPARE(textutil::countVisibleChars(QStringLiteral(u"あ\u3000い")), 2);
    QCOMPARE(textutil::countVisibleChars(QStringLiteral(u"😀😀あ")), 3);
    QCOMPARE(textutil::countVisibleChars(QStringLiteral(u"か\u3099")), 2);
    QCOMPARE(textutil::countVisibleChars(QStringLiteral(u"👨\u200d👩\u200d👧")), 5);
    QCOMPARE(textutil::countVisibleChars(QStringLiteral(u"\n \t\u3000")), 0);
    QCOMPARE(textutil::countVisibleChars(QString()), 0);
}

QTEST_APPLESS_MAIN(TestTextUtils)

#include "tst_text_utils.moc"
