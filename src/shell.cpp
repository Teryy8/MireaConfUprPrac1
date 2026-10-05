#include "shell.h"
#include "commandoptions.h"

#include <utility>

Shell::Shell(QProcessEnvironment environment)
    : parser_(std::move(environment))
{
}

QString Shell::loadVfs(const QString &path)
{
    const QString error = vfs_.load(path);
    if (error.isEmpty()) {
        directory_ = QStringLiteral("/");
        previousDirectory_.clear();
    }
    return error;
}

const Vfs &Shell::vfs() const
{
    return vfs_;
}

QString Shell::currentDirectory() const
{
    return directory_;
}

CommandResult Shell::execute(const QString &line)
{
    const QString before = directory_;
    if (!line.trimmed().isEmpty()) {
        history_.append(line);
    }
    const ParseResult parsed = parser_.parse(line);
    CommandResult result;
    if (!parsed.error.isEmpty()) {
        result = {parsed.error, true};
    } else if (!parsed.words.isEmpty()) {
        result = executeWords(parsed.words);
    }
    result.directory = before;
    return result;
}

CommandResult Shell::executeWords(const QStringList &words)
{
    const QString command = words.front();
    const QStringList arguments = words.mid(1);
    if (command == QStringLiteral("vfs-init")) {
        if (!arguments.isEmpty()) {
            return {usageError(command, command), true};
        }
        const QString error = vfs_.reset();
        if (!error.isEmpty()) {
            return {error, true};
        }
        directory_ = QStringLiteral("/");
        previousDirectory_.clear();
        return {QStringLiteral("VFS заменена на пустой корневой каталог. CSV-файл очищен.\n") + vfs_.summary()};
    }
    if (command == QStringLiteral("exit")) {
        if (!arguments.isEmpty()) {
            return {usageError(command, command), true};
        }
        return {QStringLiteral("Завершение работы эмулятора."), false, true};
    }
    if (command == QStringLiteral("ls")) {
        return listDirectory(arguments);
    }
    if (command == QStringLiteral("cd")) {
        return changeDirectory(arguments);
    }
    if (command == QStringLiteral("uniq")) {
        return uniq(arguments);
    }
    if (command == QStringLiteral("tail")) {
        return tail(arguments);
    }
    if (command == QStringLiteral("history")) {
        return history(arguments);
    }
    if (command == QStringLiteral("chmod")) {
        return chmod(arguments);
    }
    if (command == QStringLiteral("rm")) {
        return remove(arguments);
    }
    return {QStringLiteral("Ошибка: неизвестная команда «%1».").arg(command), true};
}

CommandResult Shell::history(const QStringList &arguments) const
{
    qlonglong count = history_.size();
    if (arguments.size() > 1 || (!arguments.isEmpty() && !parseCount(arguments.front(), count))) {
        return {usageError(QStringLiteral("history"), QStringLiteral("history [N]")), true};
    }
    const qsizetype start = history_.size() - qMin<qlonglong>(count, history_.size());
    QStringList lines;
    for (qsizetype i = start; i < history_.size(); ++i) {
        lines.append(QStringLiteral("%1  %2").arg(i + 1).arg(history_[i]));
    }
    return {lines.join(u'\n')};
}
