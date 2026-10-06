/*
 * Разбирает допустимые флаги и разделитель --, проверяет неотрицательные числа.
 * Формирует сообщения об ошибках аргументов с правильным синтаксисом команды.
 */

#include "commandoptions.h"

FlagOptions parseFlags(const QStringList &arguments, const QString &allowed)
{
    FlagOptions result;
    bool options = true;
    for (const QString &argument : arguments) {
        if (options && argument == QStringLiteral("--")) {
            options = false;
        } else if (options && argument.startsWith(u'-') && argument.size() > 1) {
            for (const QChar flag : argument.mid(1)) {
                if (!allowed.contains(flag)) {
                    result.valid = false;
                    return result;
                }
                result.flags += flag;
            }
        } else if (argument.isEmpty()) {
            result.valid = false;
            return result;
        } else {
            result.paths.append(argument);
        }
    }
    return result;
}

bool parseCount(const QString &text, qlonglong &count)
{
    if (text.isEmpty()) {
        return false;
    }
    for (const QChar ch : text) {
        if (ch < u'0' || ch > u'9') {
            return false;
        }
    }
    bool valid = false;
    count = text.toLongLong(&valid);
    return valid;
}

QString usageError(const QString &command, const QString &usage)
{
    return QStringLiteral("Ошибка: неверные аргументы %1. Использование: %2.").arg(command, usage);
}
