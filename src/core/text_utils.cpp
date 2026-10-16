#include "text_utils.h"

QString textutil::normalizeNewlines(const QString &text)
{
    QString out;
    out.reserve(text.size());
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (c == QLatin1Char('\r')) {
            if (i + 1 < text.size() && text.at(i + 1) == QLatin1Char('\n'))
                ++i;
            out.append(QLatin1Char('\n'));
        } else {
            out.append(c);
        }
    }
    return out;
}

int textutil::countCodePoints(const QString &text)
{
    int count = 0;
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i).isHighSurrogate() && i + 1 < text.size()
            && text.at(i + 1).isLowSurrogate()) {
            ++i;
        }
        ++count;
    }
    return count;
}

int textutil::countVisibleChars(const QString &text)
{
    int count = 0;
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (c.isHighSurrogate() && i + 1 < text.size() && text.at(i + 1).isLowSurrogate()) {
            ++i;
            ++count;
            continue;
        }
        if (c == QLatin1Char('\n') || c == QLatin1Char('\t') || c == QLatin1Char(' ')
            || c == QChar(0x3000)) {
            continue;
        }
        ++count;
    }
    return count;
}
