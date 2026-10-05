#include "mainwindow.h"

#include <QApplication>
#include <QTextStream>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Shell Emulator"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.3.0"));
    const ConfigurationResult parsed = parseConfiguration(QCoreApplication::arguments());
    if (!parsed.error.isEmpty()) {
        QTextStream(stderr) << "Ошибка параметров: " << parsed.error << Qt::endl;
        return 2;
    }
    if (!parsed.information.isEmpty()) {
        QTextStream(stdout) << parsed.information << Qt::endl;
        return 0;
    }
    QTextStream(stdout) << parsed.configuration.debugText() << Qt::endl;
    MainWindow window(parsed.configuration);
    window.show();
    QTimer::singleShot(0, &window, &MainWindow::runStartupScript);
    return app.exec();
}
