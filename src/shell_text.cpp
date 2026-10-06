/*
 * Реализует uniq и tail для текстовых файлов VFS с проверкой кодировки UTF-8.
 * uniq обрабатывает соседние повторяющиеся строки, tail выбирает последние N строк.
 */

#include "shell.h"
#include "commandoptions.h"

#include <QStringConverter>

namespace {
QStringList textLines(const QString &text)
{
    if (text.isEmpty()) {
        return {};
    }
    QStringList lines = text.split(u'\n');
    if (text.endsWith(u'\n')) {
        lines.removeLast();
    }
    return lines;
}

struct TailOptions {
    QStringList paths;
    qlonglong count = 10;
    bool valid = true;
};

TailOptions tailOptions(const QStringList &arguments)
{
    TailOptions result;
    bool options = true;
    for (qsizetype i = 0; i < arguments.size(); ++i) {
        const QString argument = arguments[i];
        if (options && argument == QStringLiteral("--")) {
            options = false;
        } else if (options && argument.startsWith(QStringLiteral("-n"))) {
            QString number = argument.mid(2);
            if (number.isEmpty() && i + 1 < arguments.size()) {
                number = arguments[++i];
            }
            if (!parseCount(number, result.count)) {
                result.valid = false;
                return result;
            }
        } else if (argument.isEmpty() || (options && argument.startsWith(u'-') && argument.size() > 1)) {
            result.valid = false;
            return result;
        } else {
            result.paths.append(argument);
        }
    }
    result.valid = !result.paths.isEmpty();
    return result;
}
} // namespace

QString Shell::readTextFile(const QString &input, QString &text) const
{
    const QString path = resolvePath(input);
    const auto node = vfs_.nodes().constFind(path);
    if (node == vfs_.nodes().cend()) {
        return QStringLiteral("файл «%1» не найден в VFS.").arg(input);
    }
    if (node->directory || input.endsWith(u'/')) {
        return QStringLiteral("«%1» не является обычным файлом.").arg(input);
    }
    QStringDecoder decoder(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
    text = decoder(node->data);
    if (decoder.hasError() || node->data.contains('\0')) {
        return QStringLiteral("файл «%1» не является текстом UTF-8.").arg(input);
    }
    return {};
}

CommandResult Shell::uniq(const QStringList &arguments) const
{
    const FlagOptions options = parseFlags(arguments, QStringLiteral("cdui"));
    if (!options.valid || options.paths.size() != 1) {
        return {usageError(QStringLiteral("uniq"), QStringLiteral("uniq [-cdui] [--] файл")), true};
    }
    QString text;
    const QString error = readTextFile(options.paths.front(), text);
    if (!error.isEmpty()) {
        return {QStringLiteral("Ошибка uniq: ") + error, true};
    }
    const QStringList lines = textLines(text);
    const Qt::CaseSensitivity sensitivity = options.flags.contains(u'i') ? Qt::CaseInsensitive : Qt::CaseSensitive;
    QStringList output;
    for (qsizetype i = 0; i < lines.size();) {
        qsizetype end = i + 1;
        while (end < lines.size() && lines[i].compare(lines[end], sensitivity) == 0) {
            ++end;
        }
        const qsizetype count = end - i;
        const bool keep = (!options.flags.contains(u'd') || count > 1) && (!options.flags.contains(u'u') || count == 1);
        if (keep) {
            output.append(options.flags.contains(u'c')
                ? QStringLiteral("%1 %2").arg(count, 7).arg(lines[i]) : lines[i]);
        }
        i = end;
    }
    return {output.isEmpty() ? QString{} : output.join(u'\n') + u'\n'};
}

CommandResult Shell::tail(const QStringList &arguments) const
{
    const TailOptions options = tailOptions(arguments);
    if (!options.valid) {
        return {usageError(QStringLiteral("tail"), QStringLiteral("tail [-n N] [--] файл ...")), true};
    }
    QString output;
    bool error = false;
    for (const QString &path : options.paths) {
        QString text;
        const QString failure = readTextFile(path, text);
        if (!failure.isEmpty()) {
            if (!output.isEmpty() && !output.endsWith(u'\n')) {
                output += u'\n';
            }
            output += QStringLiteral("Ошибка tail: ") + failure + u'\n';
            error = true;
            continue;
        }
        if (options.paths.size() > 1) {
            if (!output.isEmpty()) {
                output += output.endsWith(u'\n') ? QStringLiteral("\n") : QStringLiteral("\n\n");
            }
            output += QStringLiteral("==> %1 <==\n").arg(path);
        }
        const QStringList lines = textLines(text);
        const qsizetype count = qMin<qlonglong>(options.count, lines.size());
        if (count > 0) {
            output += lines.mid(lines.size() - count).join(u'\n');
            if (text.endsWith(u'\n')) {
                output += u'\n';
            }
        }
    }
    return {output, error};
}
