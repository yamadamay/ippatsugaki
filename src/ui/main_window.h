#pragma once

#include <QMainWindow>
#include <QTimer>

#include "core/session_controller.h"

class QComboBox;
class QLabel;
class QPushButton;

class PlainEditor;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(SessionController *controller, QWidget *parent = nullptr);

    void beginSession(const QString &promptDataPath);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onStateChanged(SessionController::State state);
    void onPromptChanged();
    void onRemainingChanged(qint64 remainingMs);
    void onSleepAutoPaused();
    void onDraftSaveFailed(const QString &message);
    void onPromptDataNotice(const QString &message);
    void onEditorTextChanged();
    void updateCharCount();
    void onTimeComboChanged(int index);
    void onRedrawClicked();
    void onConfirmClicked();
    void onPauseClicked();
    void onResumeClicked();
    void onGiveUpClicked();

private:
    void buildUi();
    void applyState(SessionController::State state);
    void startSaveFlow();
    bool askTitle(QString *title);
    bool askSavePath(const QString &title, QString *path);
    bool confirmDiscard();
    bool saveTextToFile(const QString &path, const QString &text, QString *errorOut);
    void showNotice(const QString &message);
    void updateTimerLabel(qint64 remainingMs);

    SessionController *m_controller = nullptr;
    PlainEditor *m_editor = nullptr;
    QLabel *m_promptLabel = nullptr;
    QLabel *m_elementLabel = nullptr;
    QLabel *m_timerLabel = nullptr;
    QLabel *m_charLabel = nullptr;
    QLabel *m_noticeLabel = nullptr;
    QWidget *m_chooseRow = nullptr;
    QWidget *m_writingRow = nullptr;
    QWidget *m_pausedRow = nullptr;
    QWidget *m_savingRow = nullptr;
    QPushButton *m_redrawButton = nullptr;
    QPushButton *m_confirmButton = nullptr;
    QPushButton *m_pauseButton = nullptr;
    QPushButton *m_resumeButton = nullptr;
    QPushButton *m_giveUpButton = nullptr;
    QPushButton *m_giveUpButtonPaused = nullptr;
    QComboBox *m_timeCombo = nullptr;
    QTimer m_charCountTimer;
    QTimer m_noticeTimer;
    QString m_lastTitle;
    bool m_saveFlowActive = false;
};
