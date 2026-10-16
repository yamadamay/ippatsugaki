#pragma once

#include <QString>

namespace textutil {

QString normalizeNewlines(const QString &text);
int countCodePoints(const QString &text);
int countVisibleChars(const QString &text);

}
