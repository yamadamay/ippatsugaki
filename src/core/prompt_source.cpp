#include "prompt_source.h"

#include "embedded_prompts.h"
#include "text_utils.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

namespace {

QStringList filterList(const QStringList &raw)
{
    QStringList out;
    for (const QString &item : raw) {
        if (item.trimmed().isEmpty())
            continue;
        if (textutil::countCodePoints(item) > PromptSource::maxEntryCodePoints)
            continue;
        out.append(item);
    }
    return out;
}

QStringList toStringList(const QJsonValue &value)
{
    QStringList out;
    if (!value.isArray())
        return out;
    const QJsonArray array = value.toArray();
    for (const QJsonValue &item : array) {
        if (item.isString())
            out.append(item.toString());
    }
    return out;
}

}

PromptSource::PromptSource(const PromptData &data)
    : m_data(data)
    , m_rng(std::random_device{}())
{
}

Prompt PromptSource::draw()
{
    Prompt prompt;
    prompt.subject = pick(m_data.subjects, m_lastSubject);
    prompt.event = pick(m_data.events, m_lastEvent);
    prompt.element = pick(m_data.elements, m_lastElement);
    return prompt;
}

QString PromptSource::pick(const QStringList &list, QString &last)
{
    if (list.isEmpty())
        return QString();

    QList<int> candidates;
    candidates.reserve(list.size());
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i) != last)
            candidates.append(i);
    }
    if (candidates.isEmpty()) {
        for (int i = 0; i < list.size(); ++i)
            candidates.append(i);
    }

    std::uniform_int_distribution<int> dist(0, candidates.size() - 1);
    const QString chosen = list.at(candidates.at(dist(m_rng)));
    last = chosen;
    return chosen;
}

PromptData PromptSource::embeddedData()
{
    return PromptData{embedded::subjects(), embedded::events(), embedded::elements()};
}

PromptData PromptSource::sanitize(const PromptData &raw, bool *usedEmbedded, QString *message)
{
    PromptData data;
    data.subjects = filterList(raw.subjects);
    data.events = filterList(raw.events);
    data.elements = filterList(raw.elements);

    const PromptData fallback = embeddedData();
    bool usedFallback = false;

    if (data.subjects.isEmpty()) {
        data.subjects = fallback.subjects;
        usedFallback = true;
    }
    if (data.events.isEmpty()) {
        data.events = fallback.events;
        usedFallback = true;
    }
    if (data.elements.isEmpty()) {
        data.elements = fallback.elements;
        usedFallback = true;
    }

    if (usedEmbedded)
        *usedEmbedded = usedFallback;
    if (message) {
        *message = usedFallback
                       ? QStringLiteral("お題データの問題により、内蔵データを使用しています。")
                       : QString();
    }
    return data;
}

PromptData PromptSource::loadFromFile(const QString &path, bool *usedEmbedded, QString *message)
{
    const auto fallback = [usedEmbedded, message](const QString &text) {
        if (usedEmbedded)
            *usedEmbedded = true;
        if (message)
            *message = text;
        return embeddedData();
    };

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return fallback(QStringLiteral("お題データを読み込めないため、内蔵データを使用しています。"));

    QByteArray bytes = file.readAll();
    if (bytes.startsWith("\xEF\xBB\xBF"))
        bytes.remove(0, 3);

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        return fallback(QStringLiteral("お題データの形式が不正なため、内蔵データを使用しています。"));

    const QJsonObject obj = doc.object();
    PromptData raw;
    raw.subjects = toStringList(obj.value(QStringLiteral("subjects")));
    raw.events = toStringList(obj.value(QStringLiteral("events")));
    raw.elements = toStringList(obj.value(QStringLiteral("elements")));
    return sanitize(raw, usedEmbedded, message);
}
