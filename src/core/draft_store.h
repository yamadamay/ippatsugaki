#pragma once

#include <QString>

class DraftStore
{
public:
    enum class LoadResult {
        Missing,
        Empty,
        Ok,
        Error,
    };

    explicit DraftStore(const QString &filePath);

    QString path() const;

    bool save(const QString &text, QString *errorOut = nullptr);
    LoadResult load(QString *textOut, QString *errorOut = nullptr);
    void remove();

    static QString defaultFilePath();

private:
    QString m_filePath;
};
