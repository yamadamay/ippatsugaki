#include <QApplication>

#include "core/clock.h"
#include "core/draft_store.h"
#include "core/session_controller.h"
#include "ui/main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Ippatsugaki"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    SystemClock clock;
    DraftStore draftStore(DraftStore::defaultFilePath());
    SessionController controller(&clock, &draftStore);

    MainWindow window(&controller);
    window.show();
    window.beginSession(QCoreApplication::applicationDirPath()
                        + QStringLiteral("/data/prompts.json"));

    return app.exec();
}
