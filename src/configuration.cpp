#include "configuration.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>

QString Configuration::vfsName() const
{
    const QString name = QFileInfo(vfsPath).completeBaseName();
    return name.isEmpty() ? QStringLiteral("VFS-17") : name;
}

QString Configuration::debugText() const
{
    return QStringLiteral("Параметры запуска:\nVFS: %1\nСтартовый скрипт: %2")
        .arg(vfsPath, scriptPath.isEmpty() ? QStringLiteral("не задан") : scriptPath);
}

ConfigurationResult parseConfiguration(const QStringList &arguments)
{
    ConfigurationResult result;
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Эмулятор оболочки. Вариант 17."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({QStringLiteral("vfs"), QStringLiteral("Путь к файлу VFS."),
                      QStringLiteral("path"), result.configuration.vfsPath});
    parser.addOption({QStringLiteral("script"), QStringLiteral("Путь к стартовому скрипту UTF-8."),
                      QStringLiteral("path")});
    if (!parser.parse(arguments)) {
        result.error = parser.errorText();
        return result;
    }
    if (parser.isSet(QStringLiteral("help")) || parser.isSet(QStringLiteral("help-all"))) {
        result.information = parser.helpText();
        return result;
    }
    if (parser.isSet(QStringLiteral("version"))) {
        result.information = QCoreApplication::applicationVersion();
        return result;
    }
    if (!parser.positionalArguments().isEmpty()) {
        result.error = QStringLiteral("Лишние аргументы. Используйте --vfs <путь> и --script <путь>.");
        return result;
    }
    result.configuration.vfsPath = parser.value(QStringLiteral("vfs"));
    result.configuration.scriptPath = parser.value(QStringLiteral("script"));
    if (result.configuration.vfsPath.trimmed().isEmpty()
        || (parser.isSet(QStringLiteral("script")) && result.configuration.scriptPath.trimmed().isEmpty())) {
        result.error = QStringLiteral("Путь не должен быть пустым.");
    }
    return result;
}
