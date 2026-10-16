#include "session_controller.h"

#include "text_utils.h"

SessionController::SessionController(Clock *clock, DraftStore *draftStore, QObject *parent)
    : QObject(parent)
    , m_draftStore(draftStore)
    , m_prompts(PromptSource::embeddedData())
    , m_timer(clock)
{
    m_autosaveTimer.setInterval(autosaveIntervalMs);

    connect(&m_timer, &SessionTimer::tick, this, &SessionController::remainingChanged);
    connect(&m_timer, &SessionTimer::expired, this, [this] {
        enterSaving();
        emit timedOut();
    });
    connect(&m_timer, &SessionTimer::sleepDetected, this, [this] {
        if (m_state == State::Writing) {
            pause();
            emit sleepAutoPaused();
        }
    });
    connect(&m_autosaveTimer, &QTimer::timeout, this, &SessionController::autosaveTick);
}

void SessionController::start(const QString &promptDataPath)
{
    bool usedEmbedded = false;
    QString message;
    const PromptData data = PromptSource::loadFromFile(promptDataPath, &usedEmbedded, &message);
    if (usedEmbedded)
        emit promptDataNotice(message);
    startWithData(data);
}

void SessionController::startWithData(const PromptData &data)
{
    m_prompts = PromptSource(data);
    m_state = State::ChoosePrompt;
    m_text.clear();
    m_dirty = false;
    m_timeLimitMinutes = defaultTimeLimitMinutes;
    redrawPrompt();
}

SessionController::State SessionController::state() const
{
    return m_state;
}

qint64 SessionController::remainingMs() const
{
    if (m_state == State::ChoosePrompt)
        return qint64(m_timeLimitMinutes) * 60000;
    return m_timer.remainingMs();
}

Prompt SessionController::currentPrompt() const
{
    return m_prompt;
}

QString SessionController::documentText() const
{
    return m_text;
}

int SessionController::timeLimitMinutes() const
{
    return m_timeLimitMinutes;
}

void SessionController::setTimeLimitMinutes(int minutes)
{
    if (m_state != State::ChoosePrompt)
        return;
    m_timeLimitMinutes = minutes;
    emit remainingChanged(remainingMs());
}

void SessionController::redrawPrompt()
{
    if (m_state != State::ChoosePrompt)
        return;
    m_prompt = m_prompts.draw();
    emit promptChanged();
    emit remainingChanged(remainingMs());
}

bool SessionController::confirmStart(QString *errorOut)
{
    if (m_state != State::ChoosePrompt)
        return false;

    m_text.clear();
    m_dirty = false;

    QString error;
    if (!m_draftStore->save(m_text, &error)) {
        if (errorOut)
            *errorOut = error;
        if (!m_draftFailureNotified) {
            m_draftFailureNotified = true;
            emit draftSaveFailed(error.isEmpty() ? QStringLiteral("一時保存に失敗しました。") : error);
        }
    } else if (errorOut) {
        errorOut->clear();
    }

    setState(State::Writing);
    m_timer.start(qint64(m_timeLimitMinutes) * 60000);
    m_autosaveTimer.start();
    return true;
}

void SessionController::pause()
{
    if (m_state != State::Writing)
        return;
    saveDraftNow();
    m_timer.pause();
    setState(State::Paused);
}

void SessionController::resume()
{
    if (m_state != State::Paused)
        return;
    setState(State::Writing);
    m_timer.resume();
}

void SessionController::giveUp()
{
    if (m_state != State::Writing && m_state != State::Paused)
        return;

    if (textutil::countVisibleChars(m_text) == 0) {
        m_draftStore->remove();
        returnToChoosePrompt();
        return;
    }

    enterSaving();
}

void SessionController::noteDocumentChanged(const QString &text)
{
    if (m_state != State::Writing && m_state != State::Paused)
        return;
    if (m_text == text)
        return;
    m_text = text;
    m_dirty = true;
}

void SessionController::finishSaveSuccess()
{
    if (m_state != State::Saving)
        return;
    m_draftStore->remove();
    returnToChoosePrompt();
}

void SessionController::finishSaveDiscarded()
{
    if (m_state != State::Saving)
        return;
    m_draftStore->remove();
    returnToChoosePrompt();
}

void SessionController::handleTick()
{
    m_timer.onTick();
}

void SessionController::autosaveTick()
{
    if (m_state != State::Writing || !m_dirty)
        return;
    saveDraftNow();
}

SessionController::RecoveryCheck SessionController::checkDraft(QString *textOut)
{
    QString error;
    const DraftStore::LoadResult result = m_draftStore->load(textOut, &error);
    switch (result) {
    case DraftStore::LoadResult::Missing:
        return RecoveryCheck::Nothing;
    case DraftStore::LoadResult::Empty:
        m_draftStore->remove();
        return RecoveryCheck::Nothing;
    case DraftStore::LoadResult::Ok:
        return RecoveryCheck::Recoverable;
    case DraftStore::LoadResult::Error:
        return RecoveryCheck::Unreadable;
    }
    return RecoveryCheck::Nothing;
}

void SessionController::recoverInto(const QString &text)
{
    m_text = text;
    m_dirty = false;
    m_autosaveTimer.stop();
    setState(State::Saving);
}

void SessionController::discardDraft()
{
    m_draftStore->remove();
}

void SessionController::setState(State state)
{
    m_state = state;
    emit stateChanged(m_state);
}

void SessionController::enterSaving()
{
    if (m_state != State::Writing && m_state != State::Paused)
        return;
    saveDraftNow();
    m_autosaveTimer.stop();
    m_timer.pause();
    setState(State::Saving);
}

void SessionController::returnToChoosePrompt()
{
    m_text.clear();
    m_dirty = false;
    m_autosaveTimer.stop();
    m_timer.stop();
    m_timeLimitMinutes = defaultTimeLimitMinutes;
    setState(State::ChoosePrompt);
    redrawPrompt();
}

bool SessionController::saveDraftNow()
{
    QString error;
    if (!m_draftStore->save(m_text, &error)) {
        if (!m_draftFailureNotified) {
            m_draftFailureNotified = true;
            emit draftSaveFailed(error.isEmpty() ? QStringLiteral("一時保存に失敗しました。") : error);
        }
        return false;
    }
    m_dirty = false;
    return true;
}
