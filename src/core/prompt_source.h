#pragma once

#include <QString>
#include <QStringList>

#include <random>

struct Prompt
{
    QString subject;
    QString event;
    QString element;
};

struct PromptData
{
    QStringList subjects;
    QStringList events;
    QStringList elements;
};

class PromptSource
{
public:
    static constexpr int maxEntryCodePoints = 100;

    explicit PromptSource(const PromptData &data);

    Prompt draw();

    static PromptData embeddedData();
    static PromptData sanitize(const PromptData &raw, bool *usedEmbedded, QString *message);
    static PromptData loadFromFile(const QString &path, bool *usedEmbedded, QString *message);

private:
    QString pick(const QStringList &list, QString &last);

    PromptData m_data;
    std::mt19937 m_rng;
    QString m_lastSubject;
    QString m_lastEvent;
    QString m_lastElement;
};
