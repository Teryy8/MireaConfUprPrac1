#include "shell.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <utility>

namespace {
QString argumentError(const QString &command)
{
    const QString usage = command == QStringLiteral("ls")
        ? QStringLiteral("ls [-a] [-l] [--] [путь ...]")
        : QStringLiteral("cd [--] [путь]");
    return QStringLiteral("Ошибка: неверные аргументы %1. Использование: %2.").arg(command, usage);
}

bool validArguments(const QString &command, const QStringList &arguments)
{
    bool options = true;
    int paths = 0;
    for (const QString &argument : arguments) {
        if (options && argument == QStringLiteral("--")) {
            options = false;
            continue;
        }
        if (options && argument.startsWith(u'-') && argument.size() > 1) {
            if (command != QStringLiteral("ls")) {
                return false;
            }
            for (const QChar flag : argument.mid(1)) {
                if (flag != u'a' && flag != u'l') {
                    return false;
                }
            }
        } else {
            if (argument.isEmpty()) {
                return false;
            }
            ++paths;
        }
    }
    return command == QStringLiteral("ls") || paths <= 1;
}

QString stubOutput(const QString &command, const QStringList &arguments)
{
    const QJsonDocument json(QJsonArray::fromStringList(arguments));
    return QStringLiteral("Заглушка: %1\nАргументы: %2")
        .arg(command, QString::fromUtf8(json.toJson(QJsonDocument::Compact)));
}
} // namespace

Shell::Shell(QProcessEnvironment environment)
    : parser_(std::move(environment))
{
}

CommandResult Shell::execute(const QString &line) const
{
    const ParseResult parsed = parser_.parse(line);
    if (!parsed.error.isEmpty()) {
        return {parsed.error, true, false};
    }
    if (parsed.words.isEmpty()) {
        return {};
    }

    const QString command = parsed.words.front();
    const QStringList arguments = parsed.words.mid(1);
    if (command == QStringLiteral("exit")) {
        if (!arguments.isEmpty()) {
            return {QStringLiteral("Ошибка: неверные аргументы exit. Использование: exit."), true, false};
        }
        return {QStringLiteral("Завершение работы эмулятора."), false, true};
    }
    if (command == QStringLiteral("ls") || command == QStringLiteral("cd")) {
        if (!validArguments(command, arguments)) {
            return {argumentError(command), true, false};
        }
        return {stubOutput(command, arguments), false, false};
    }
    return {QStringLiteral("Ошибка: неизвестная команда «%1».").arg(command), true, false};
}
