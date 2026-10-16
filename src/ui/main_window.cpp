#include "main_window.h"

#include "plain_editor.h"

#include "core/text_utils.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStatusBar>
#include <QVBoxLayout>

namespace {

const QList<int> kTimeLimits = {10, 20, 30, 45, 60};

QPushButton *makeButton(const QString &text)
{
    auto *button = new QPushButton(text);
    button->setMinimumWidth(120);
    return button;
}

bool askYesNo(QWidget *parent, const QString &title, const QString &text,
              const QString &yesText, const QString &noText, bool defaultYes)
{
    QMessageBox box(QMessageBox::Question, title, text, QMessageBox::NoButton, parent);
    QPushButton *yesButton = box.addButton(yesText, QMessageBox::AcceptRole);
    QPushButton *noButton = box.addButton(noText, QMessageBox::RejectRole);
    box.setDefaultButton(defaultYes ? yesButton : noButton);
    box.exec();
    return box.clickedButton() == yesButton;
}

QString sanitizeFileName(const QString &title)
{
    static const QString forbidden = QStringLiteral("<>:\"/\\|?*");
    QString out;
    out.reserve(title.size());
    for (const QChar c : title) {
        if (c.unicode() < 0x20 || forbidden.contains(c))
            out.append(QLatin1Char('_'));
        else
            out.append(c);
    }
    while (out.endsWith(QLatin1Char('.')) || out.endsWith(QLatin1Char(' ')))
        out.chop(1);
    if (out.isEmpty())
        out = QStringLiteral("無題");
    return out;
}

QString formatRemaining(qint64 remainingMs)
{
    const qint64 seconds = (remainingMs + 999) / 1000;
    const qint64 minutes = seconds / 60;
    const qint64 rest = seconds % 60;
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(rest, 2, 10, QLatin1Char('0'));
}

}

MainWindow::MainWindow(SessionController *controller, QWidget *parent)
    : QMainWindow(parent)
    , m_controller(controller)
{
    setWindowTitle(QStringLiteral("一発書き"));
    resize(900, 700);

    buildUi();

    m_charCountTimer.setSingleShot(true);
    m_charCountTimer.setInterval(200);
    connect(&m_charCountTimer, &QTimer::timeout, this, &MainWindow::updateCharCount);

    m_noticeTimer.setSingleShot(true);
    m_noticeTimer.setInterval(10000);
    connect(&m_noticeTimer, &QTimer::timeout, this, [this] { m_noticeLabel->clear(); });

    connect(m_controller, &SessionController::stateChanged, this, &MainWindow::onStateChanged);
    connect(m_controller, &SessionController::promptChanged, this, &MainWindow::onPromptChanged);
    connect(m_controller, &SessionController::remainingChanged, this, &MainWindow::onRemainingChanged);
    connect(m_controller, &SessionController::sleepAutoPaused, this, &MainWindow::onSleepAutoPaused);
    connect(m_controller, &SessionController::draftSaveFailed, this, &MainWindow::onDraftSaveFailed);
    connect(m_controller, &SessionController::promptDataNotice, this, &MainWindow::onPromptDataNotice);

    applyState(m_controller->state());
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QVBoxLayout(central);

    auto *topRow = new QHBoxLayout();
    m_promptLabel = new QLabel();
    QFont promptFont = m_promptLabel->font();
    promptFont.setPointSize(promptFont.pointSize() + 6);
    promptFont.setBold(true);
    m_promptLabel->setFont(promptFont);
    m_promptLabel->setWordWrap(true);
    topRow->addWidget(m_promptLabel, 1);

    m_timerLabel = new QLabel();
    QFont timerFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    timerFont.setPointSize(timerFont.pointSize() + 10);
    timerFont.setBold(true);
    m_timerLabel->setFont(timerFont);
    m_timerLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    topRow->addWidget(m_timerLabel);
    layout->addLayout(topRow);

    auto *subRow = new QHBoxLayout();
    m_elementLabel = new QLabel();
    QFont elementFont = m_elementLabel->font();
    elementFont.setPointSize(elementFont.pointSize() + 2);
    m_elementLabel->setFont(elementFont);
    subRow->addWidget(m_elementLabel, 1);

    m_charLabel = new QLabel();
    m_charLabel->setAlignment(Qt::AlignRight);
    subRow->addWidget(m_charLabel);
    layout->addLayout(subRow);

    m_editor = new PlainEditor();
    layout->addWidget(m_editor, 1);
    connect(m_editor, &QPlainTextEdit::textChanged, this, &MainWindow::onEditorTextChanged);

    m_chooseRow = new QWidget();
    auto *chooseLayout = new QHBoxLayout(m_chooseRow);
    chooseLayout->setContentsMargins(0, 0, 0, 0);
    m_redrawButton = makeButton(QStringLiteral("引き直し"));
    chooseLayout->addWidget(m_redrawButton);
    chooseLayout->addWidget(new QLabel(QStringLiteral("制限時間:")));
    m_timeCombo = new QComboBox();
    for (int minutes : kTimeLimits)
        m_timeCombo->addItem(QStringLiteral("%1分").arg(minutes), minutes);
    m_timeCombo->setCurrentIndex(kTimeLimits.indexOf(SessionController::defaultTimeLimitMinutes));
    chooseLayout->addWidget(m_timeCombo);
    chooseLayout->addStretch(1);
    m_confirmButton = makeButton(QStringLiteral("確定"));
    chooseLayout->addWidget(m_confirmButton);

    m_writingRow = new QWidget();
    auto *writingLayout = new QHBoxLayout(m_writingRow);
    writingLayout->setContentsMargins(0, 0, 0, 0);
    m_pauseButton = makeButton(QStringLiteral("一時停止"));
    writingLayout->addWidget(m_pauseButton);
    writingLayout->addStretch(1);
    m_giveUpButton = makeButton(QStringLiteral("ギブアップ"));
    writingLayout->addWidget(m_giveUpButton);

    m_pausedRow = new QWidget();
    auto *pausedLayout = new QHBoxLayout(m_pausedRow);
    pausedLayout->setContentsMargins(0, 0, 0, 0);
    m_resumeButton = makeButton(QStringLiteral("再開"));
    pausedLayout->addWidget(m_resumeButton);
    pausedLayout->addStretch(1);
    m_giveUpButtonPaused = makeButton(QStringLiteral("ギブアップ"));
    pausedLayout->addWidget(m_giveUpButtonPaused);

    m_savingRow = new QWidget();
    auto *savingLayout = new QHBoxLayout(m_savingRow);
    savingLayout->setContentsMargins(0, 0, 0, 0);
    savingLayout->addWidget(new QLabel(QStringLiteral("保存してください。")));
    savingLayout->addStretch(1);

    layout->addWidget(m_chooseRow);
    layout->addWidget(m_writingRow);
    layout->addWidget(m_pausedRow);
    layout->addWidget(m_savingRow);

    m_noticeLabel = new QLabel();
    statusBar()->addPermanentWidget(m_noticeLabel);

    connect(m_redrawButton, &QPushButton::clicked, this, &MainWindow::onRedrawClicked);
    connect(m_confirmButton, &QPushButton::clicked, this, &MainWindow::onConfirmClicked);
    connect(m_pauseButton, &QPushButton::clicked, this, &MainWindow::onPauseClicked);
    connect(m_resumeButton, &QPushButton::clicked, this, &MainWindow::onResumeClicked);
    connect(m_giveUpButton, &QPushButton::clicked, this, &MainWindow::onGiveUpClicked);
    connect(m_giveUpButtonPaused, &QPushButton::clicked, this, &MainWindow::onGiveUpClicked);
    connect(m_timeCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onTimeComboChanged);
}

void MainWindow::beginSession(const QString &promptDataPath)
{
    m_controller->start(promptDataPath);

    for (;;) {
        QString text;
        const SessionController::RecoveryCheck check = m_controller->checkDraft(&text);
        if (check == SessionController::RecoveryCheck::Nothing)
            break;
        if (check == SessionController::RecoveryCheck::Unreadable) {
            QMessageBox::warning(this, QStringLiteral("一時保存ファイル"),
                                 QStringLiteral("一時保存ファイルを読み込めませんでした。"
                                                "ファイルは削除せずに続けます。"));
            break;
        }

        if (askYesNo(this, QStringLiteral("復旧"),
                     QStringLiteral("前回の文章が残っています。復旧しますか？"),
                     QStringLiteral("はい"), QStringLiteral("いいえ"), true)) {
            m_editor->setPlainText(text);
            m_controller->recoverInto(text);
            break;
        }

        if (askYesNo(this, QStringLiteral("削除"),
                     QStringLiteral("二度と復活できません。削除しますか？"),
                     QStringLiteral("はい"), QStringLiteral("いいえ"), false)) {
            m_controller->discardDraft();
            break;
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    switch (m_controller->state()) {
    case SessionController::State::ChoosePrompt:
        event->accept();
        return;
    case SessionController::State::Writing:
    case SessionController::State::Paused:
        if (askYesNo(this, QStringLiteral("ギブアップ"),
                     QStringLiteral("ギブアップして、ここまでの文章を保存します。よろしいですか？"),
                     QStringLiteral("はい"), QStringLiteral("いいえ"), false)) {
            m_controller->giveUp();
        }
        event->ignore();
        return;
    case SessionController::State::Saving:
        statusBar()->showMessage(QStringLiteral("保存を完了してください"), 5000);
        event->ignore();
        return;
    }
}

void MainWindow::onStateChanged(SessionController::State state)
{
    applyState(state);
    if (state == SessionController::State::Saving)
        startSaveFlow();
}

void MainWindow::onPromptChanged()
{
    const Prompt prompt = m_controller->currentPrompt();
    m_promptLabel->setText(QStringLiteral("%1が%2。").arg(prompt.subject, prompt.event));
    m_elementLabel->setText(QStringLiteral("必須要素：%1").arg(prompt.element));
}

void MainWindow::onRemainingChanged(qint64 remainingMs)
{
    updateTimerLabel(remainingMs);
}

void MainWindow::onSleepAutoPaused()
{
    statusBar()->showMessage(QStringLiteral("PCの停止を検出したため一時停止しました"));
}

void MainWindow::onDraftSaveFailed(const QString &message)
{
    showNotice(message);
}

void MainWindow::onPromptDataNotice(const QString &message)
{
    showNotice(message);
}

void MainWindow::onEditorTextChanged()
{
    m_controller->noteDocumentChanged(m_editor->toPlainText());
    m_charCountTimer.start();
}

void MainWindow::updateCharCount()
{
    const int count = textutil::countVisibleChars(m_editor->toPlainText());
    m_charLabel->setText(QStringLiteral("文字数: %1").arg(count));
}

void MainWindow::onTimeComboChanged(int index)
{
    const int minutes = m_timeCombo->itemData(index).toInt();
    m_controller->setTimeLimitMinutes(minutes);
}

void MainWindow::onRedrawClicked()
{
    m_controller->redrawPrompt();
}

void MainWindow::onConfirmClicked()
{
    if (!askYesNo(this, QStringLiteral("確定"),
                  QStringLiteral("確定すると時間が始まり、お題の変更はできません。"
                                 "やめるにはギブアップのみです。"),
                  QStringLiteral("確定して開始"), QStringLiteral("戻る"), false)) {
        return;
    }
    m_controller->confirmStart();
}

void MainWindow::onPauseClicked()
{
    m_controller->pause();
}

void MainWindow::onResumeClicked()
{
    m_controller->resume();
}

void MainWindow::onGiveUpClicked()
{
    if (!askYesNo(this, QStringLiteral("ギブアップ"),
                  QStringLiteral("ギブアップして、ここまでの文章を保存します。よろしいですか？"),
                  QStringLiteral("はい"), QStringLiteral("いいえ"), false)) {
        return;
    }
    m_controller->giveUp();
}

void MainWindow::applyState(SessionController::State state)
{
    m_chooseRow->setVisible(state == SessionController::State::ChoosePrompt);
    m_writingRow->setVisible(state == SessionController::State::Writing);
    m_pausedRow->setVisible(state == SessionController::State::Paused);
    m_savingRow->setVisible(state == SessionController::State::Saving);

    switch (state) {
    case SessionController::State::ChoosePrompt:
        m_editor->clear();
        m_editor->setReadOnly(true);
        m_charLabel->setText(QStringLiteral("文字数: 0"));
        m_timeCombo->setCurrentIndex(kTimeLimits.indexOf(SessionController::defaultTimeLimitMinutes));
        statusBar()->clearMessage();
        m_noticeLabel->clear();
        break;
    case SessionController::State::Writing:
        m_editor->setReadOnly(false);
        m_editor->setFocus();
        statusBar()->clearMessage();
        break;
    case SessionController::State::Paused:
        m_editor->setReadOnly(true);
        break;
    case SessionController::State::Saving:
        m_editor->setReadOnly(true);
        statusBar()->clearMessage();
        break;
    }
}

void MainWindow::startSaveFlow()
{
    if (m_saveFlowActive)
        return;
    m_saveFlowActive = true;

    for (;;) {
        QString title = m_lastTitle;
        if (!askTitle(&title)) {
            if (confirmDiscard()) {
                m_controller->finishSaveDiscarded();
                break;
            }
            continue;
        }
        m_lastTitle = title;

        QString path;
        if (!askSavePath(title, &path)) {
            if (confirmDiscard()) {
                m_controller->finishSaveDiscarded();
                break;
            }
            continue;
        }

        QString error;
        if (saveTextToFile(path, m_controller->documentText(), &error)) {
            m_controller->finishSaveSuccess();
            break;
        }

        QMessageBox::critical(this, QStringLiteral("保存エラー"),
                              QStringLiteral("保存に失敗しました。\n%1").arg(error));
    }

    m_saveFlowActive = false;
}

bool MainWindow::askTitle(QString *title)
{
    for (;;) {
        bool ok = false;
        const QString text = QInputDialog::getText(this, QStringLiteral("タイトル"),
                                                   QStringLiteral("タイトルを入力してください（必須）"),
                                                   QLineEdit::Normal, *title, &ok);
        if (!ok)
            return false;
        if (!text.trimmed().isEmpty()) {
            *title = text;
            return true;
        }
        QMessageBox::information(this, QStringLiteral("タイトル"),
                                 QStringLiteral("タイトルを入力してください。"));
    }
}

bool MainWindow::askSavePath(const QString &title, QString *path)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    const QString suggested = dir + QLatin1Char('/') + sanitizeFileName(title)
                              + QStringLiteral(".txt");
    QString chosen = QFileDialog::getSaveFileName(this, QStringLiteral("保存"), suggested,
                                                  QStringLiteral("テキストファイル (*.txt)"));
    if (chosen.isEmpty())
        return false;
    if (!chosen.endsWith(QStringLiteral(".txt"), Qt::CaseInsensitive))
        chosen += QStringLiteral(".txt");
    *path = chosen;
    return true;
}

bool MainWindow::confirmDiscard()
{
    if (!askYesNo(this, QStringLiteral("確認"),
                  QStringLiteral("保存しないと、書いた文章は保存できなくなります。"
                                 "保存せずに終わりますか？"),
                  QStringLiteral("はい"), QStringLiteral("いいえ"), false)) {
        return false;
    }
    return askYesNo(this, QStringLiteral("確認"),
                    QStringLiteral("本当に削除されます。元に戻せません。よろしいですか？"),
                    QStringLiteral("はい"), QStringLiteral("いいえ"), false);
}

bool MainWindow::saveTextToFile(const QString &path, const QString &text, QString *errorOut)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        *errorOut = file.errorString();
        return false;
    }

    const QByteArray bytes = text.toUtf8();
    if (file.write(bytes) != bytes.size()) {
        *errorOut = file.errorString();
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        *errorOut = file.errorString();
        return false;
    }

    return true;
}

void MainWindow::showNotice(const QString &message)
{
    m_noticeLabel->setText(message);
    m_noticeTimer.start();
}

void MainWindow::updateTimerLabel(qint64 remainingMs)
{
    m_timerLabel->setText(formatRemaining(remainingMs));
}
