#include "plain_editor.h"

#include "core/text_utils.h"

#include <QMimeData>

PlainEditor::PlainEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
}

void PlainEditor::insertFromMimeData(const QMimeData *source)
{
    if (source->hasText()) {
        insertPlainText(textutil::normalizeNewlines(source->text()));
        return;
    }
    QPlainTextEdit::insertFromMimeData(source);
}
