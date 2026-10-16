#pragma once

#include <QPlainTextEdit>

class PlainEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit PlainEditor(QWidget *parent = nullptr);

protected:
    void insertFromMimeData(const QMimeData *source) override;
};
