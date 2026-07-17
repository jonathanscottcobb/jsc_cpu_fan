#include "HwmonReader.h"
#include "TrayController.h"

#include <QApplication>
#include <QMessageBox>
#include <QSystemTrayIcon>

#include <cstring>

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--dump") == 0) {
            HwmonReader reader;
            reader.dumpToStdout();
            return 0;
        }
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("jsc_cpu"));
    QApplication::setApplicationDisplayName(QStringLiteral("JSC CPU Temp & Fan Monitor"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setOrganizationName(QStringLiteral("Jonathan Scott-Cobb"));
    QApplication::setQuitOnLastWindowClosed(false);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, QStringLiteral("JSC CPU"),
                              QStringLiteral("System tray is not available on this desktop."));
        return 1;
    }

    TrayController tray;
    Q_UNUSED(tray);

    return app.exec();
}
