#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include "clock.h"
#include "draft_store.h"
#include "prompt_source.h"
#include "session_timer.h"

class SessionController : public QObject
{
    Q_OBJECT

public:
    enum class State {
        ChoosePrompt,
        Writing,
        Paused,
        Saving,
    };
    Q_ENUM(State)

    enum class RecoveryCheck {
        Nothing,
        Recoverable,
        Unreadable,
    };

    static constexpr int autosaveIntervalMs = 30000;
    static constexpr int defaultTimeLimitMinutes = 30;

    SessionController(Clock *clock, DraftStore *draftStore, QObject *parent = nullptr);

    void start(const QString &promptDataPath);
    void startWithData(const PromptData &data);

    State state() const;
    qint64 remainingMs() const;
    Prompt currentPrompt() const;
    QString documentText() const;
    int timeLimitMinutes() const;

    void setTimeLimitMinutes(int minutes);
    void redrawPrompt();
    bool confirmStart(QString *errorOut = nullptr);
    void pause();
    void resume();
    void giveUp();
    void noteDocumentChanged(const QString &text);
    void finishSaveSuccess();
    void finishSaveDiscarded();
    void handleTick();
    void autosaveTick();

    RecoveryCheck checkDraft(QString *textOut);
    void recoverInto(const QString &text);
    void discardDraft();

signals:
    void stateChanged(SessionController::State state);
    void promptChanged();
    void remainingChanged(qint64 remainingMs);
    void sleepAutoPaused();
    void timedOut();
    void draftSaveFailed(const QString &message);
    void promptDataNotice(const QString &message);

private:
    void setState(State state);
    void enterSaving();
    void returnToChoosePrompt();
    bool saveDraftNow();

    DraftStore *m_draftStore = nullptr;
    PromptSource m_prompts;
    SessionTimer m_timer;
    QTimer m_autosaveTimer;
    State m_state = State::ChoosePrompt;
    Prompt m_prompt;
    QString m_text;
    bool m_dirty = false;
    bool m_draftFailureNotified = false;
    int m_timeLimitMinutes = defaultTimeLimitMinutes;
};
