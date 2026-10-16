#include "draft_store.h"

#include "text_utils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringDecoder>

DraftStore::DraftStore(const QString &filePath)
    : m_filePath(filePath)
{
}

QString DraftStore::path() const
{
    return m_filePath;
}

QString DraftStore::defaultFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
           + QStringLiteral("/draft.txt");
}

bool DraftStore::save(const QString &text, QString *errorOut)
{
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());

    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorOut)
            *errorOut = file.errorString();
        return false;
    }

    const QByteArray bytes = text.toUtf8();
    if (file.write(bytes) != bytes.size()) {
        if (errorOut)
            *errorOut = file.errorString();
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        if (errorOut)
            *errorOut = file.errorString();
        return false;
    }

    return true;
}

DraftStore::LoadResult DraftStore::load(QString *textOut, QString *errorOut)
{
    if (textOut)
        textOut->clear();

    QFile file(m_filePath);
    if (!file.exists())
        return LoadResult::Missing;

    if (!file.open(QIODevice::ReadOnly)) {
        if (errorOut)
            *errorOut = file.errorString();
        return LoadResult::Error;
    }

    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        if (errorOut)
            *errorOut = file.errorString();
        return LoadResult::Error;
    }

    QStringDecoder decoder(QStringDecoder::Utf8);
    QString text = decoder.decode(bytes);
    if (decoder.hasError()) {
        if (errorOut)
            *errorOut = QStringLiteral("UTF-8として読み込めません。");
        return LoadResult::Error;
    }

    if (text.startsWith(QChar(0xFEFF)))
        text.remove(0, 1);
    text = textutil::normalizeNewlines(text);

    if (text.isEmpty())
        return LoadResult::Empty;

    if (textOut)
        *textOut = text;
    return LoadResult::Ok;
}

void DraftStore::remove()
{
    QFile::remove(m_filePath);
}
