#include "shell.h"
#include "commandoptions.h"
#include "permissions.h"

CommandResult Shell::chmod(const QStringList &arguments)
{
    const QString usage = QStringLiteral("chmod [-Rv] режим [--] путь ...");
    bool recursive = false;
    bool verbose = false;
    qsizetype position = 0;
    // Флаги до режима; -w и другие символьные режимы начинаются с минуса.
    while (position < arguments.size()) {
        const QString &argument = arguments[position];
        if (argument == QStringLiteral("--")) {
            ++position;
            break;
        }
        if (!argument.startsWith(u'-') || argument.size() < 2) break;
        bool flag = true;
        for (QChar ch : argument.mid(1)) {
            if (ch != u'R' && ch != u'v') flag = false;
        }
        if (!flag) break;
        recursive |= argument.contains(u'R');
        verbose |= argument.contains(u'v');
        ++position;
    }
    PermissionMode mode;
    if (position >= arguments.size() || !mode.parse(arguments[position++])) {
        return {usageError(QStringLiteral("chmod"), usage), true};
    }
    const FlagOptions options = parseFlags(arguments.mid(position), {});
    if (!options.valid || options.paths.isEmpty()) {
        return {usageError(QStringLiteral("chmod"), usage), true};
    }
    QStringList output;
    bool error = false;
    for (const QString &input : options.paths) {
        const QString path = resolvePath(input);
        const auto node = vfs_.nodes().constFind(path);
        if (node == vfs_.nodes().cend()) {
            output.append(QStringLiteral("Ошибка chmod: путь «%1» не найден в VFS.").arg(input));
            error = true;
            continue;
        }
        if (!node->directory && input.endsWith(u'/')) {
            output.append(QStringLiteral("Ошибка chmod: «%1» не является каталогом.").arg(input));
            error = true;
            continue;
        }
        QStringList paths{path};
        if (recursive && node->directory) {
            const QString prefix = path == QStringLiteral("/") ? path : path + u'/';
            for (const QString &key : vfs_.nodes().keys()) {
                if (key != path && key.startsWith(prefix)) paths.append(key);
            }
        }
        for (const QString &key : paths) {
            const VfsNode current = vfs_.nodes().value(key);
            const unsigned int permissions = mode.apply(current.permissions, current.directory);
            vfs_.setPermissions(key, permissions);
            if (verbose) {
                output.append(QStringLiteral("chmod: «%1» %2 -> %3")
                    .arg(key, QString::number(current.permissions, 8).rightJustified(4, u'0'),
                         QString::number(permissions, 8).rightJustified(4, u'0')));
            }
        }
    }
    return {output.join(u'\n'), error};
}

CommandResult Shell::remove(const QStringList &arguments)
{
    const FlagOptions options = parseFlags(arguments, QStringLiteral("rRfdv"));
    const bool force = options.flags.contains(u'f');
    if (!options.valid || (options.paths.isEmpty() && !force)) {
        return {usageError(QStringLiteral("rm"), QStringLiteral("rm [-rRfdv] [--] путь ...")), true};
    }
    const bool recursive = options.flags.contains(u'r') || options.flags.contains(u'R');
    QStringList output;
    bool error = false;
    for (const QString &input : options.paths) {
        const QString path = resolvePath(input);
        const QStringList parts = input.split(u'/', Qt::SkipEmptyParts);
        const QString last = parts.isEmpty() ? QString{} : parts.back();
        QString failure;
        if (last == QStringLiteral(".") || last == QStringLiteral("..")) {
            failure = QStringLiteral("нельзя удалять . или ..");
        } else if (path == QStringLiteral("/")) {
            failure = QStringLiteral("нельзя удалить корень VFS");
        } else if (directory_ == path || directory_.startsWith(path + u'/')) {
            failure = QStringLiteral("нельзя удалить текущий каталог или его родителя");
        } else if (!vfs_.nodes().contains(path) && force) {
            continue;
        } else if (vfs_.nodes().contains(path) && !vfs_.nodes().value(path).directory && input.endsWith(u'/')) {
            failure = QStringLiteral("путь не является каталогом");
        } else {
            failure = vfs_.remove(path, recursive, options.flags.contains(u'd'));
        }
        if (!failure.isEmpty()) {
            if (failure.endsWith(u'.')) failure.chop(1);
            output.append(QStringLiteral("Ошибка rm: «%1»: %2.").arg(input, failure));
            error = true;
        } else {
            if (previousDirectory_ == path || previousDirectory_.startsWith(path + u'/')) {
                previousDirectory_.clear();
            }
            if (options.flags.contains(u'v')) output.append(QStringLiteral("rm: удалён «%1»").arg(path));
        }
    }
    return {output.join(u'\n'), error};
}
