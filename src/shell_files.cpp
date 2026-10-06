/*
 * Реализует ls и cd, а также преобразование относительных путей в абсолютные.
 * Обрабатывает . и .., формирует список элементов и вывод прав и размеров.
 */

#include "shell.h"
#include "commandoptions.h"

namespace {
QString parentPath(const QString &path)
{
    const QString parent = path.left(path.lastIndexOf(u'/'));
    return parent.isEmpty() ? QStringLiteral("/") : parent;
}

QString permissionString(const VfsNode &node)
{
    QString result = node.directory ? QStringLiteral("d") : QStringLiteral("-");
    const QString symbols = QStringLiteral("rwx");
    for (int bit = 8; bit >= 0; --bit) {
        result += (node.permissions & (1u << bit)) ? symbols[(8 - bit) % 3] : u'-';
    }
    if (node.permissions & 04000) {
        result[3] = node.permissions & 0100 ? u's' : u'S';
    }
    if (node.permissions & 02000) {
        result[6] = node.permissions & 0010 ? u's' : u'S';
    }
    if (node.permissions & 01000) {
        result[9] = node.permissions & 0001 ? u't' : u'T';
    }
    return result;
}

QString entryText(const QString &name, const VfsNode &node, bool details)
{
    if (!details) {
        return name;
    }
    const qsizetype size = node.directory ? 0 : node.data.size();
    return QStringLiteral("%1 %2 %3").arg(permissionString(node)).arg(size).arg(name);
}
} // namespace

QString Shell::resolvePath(const QString &path) const
{
    const QString absolute = path.startsWith(u'/') ? path : directory_ + u'/' + path;
    QStringList parts;
    for (const QString &part : absolute.split(u'/', Qt::SkipEmptyParts)) {
        if (part == QStringLiteral("..")) {
            if (!parts.isEmpty()) {
                parts.removeLast();
            }
        } else if (part != QStringLiteral(".")) {
            parts.append(part);
        }
    }
    return u'/' + parts.join(u'/');
}

CommandResult Shell::changeDirectory(const QStringList &arguments)
{
    const FlagOptions options = parseFlags(arguments, {});
    if (!options.valid || options.paths.size() > 1) {
        return {usageError(QStringLiteral("cd"), QStringLiteral("cd [--] [путь | -]")), true};
    }
    const QString input = options.paths.isEmpty() ? QStringLiteral("/") : options.paths.front();
    const bool previous = input == QStringLiteral("-");
    if (previous && previousDirectory_.isEmpty()) {
        return {QStringLiteral("Ошибка cd: предыдущий каталог не задан."), true};
    }
    const QString target = previous ? previousDirectory_ : resolvePath(input);
    const auto node = vfs_.nodes().constFind(target);
    if (node == vfs_.nodes().cend()) {
        return {QStringLiteral("Ошибка cd: каталог «%1» не найден в VFS.").arg(input), true};
    }
    if (!node->directory) {
        return {QStringLiteral("Ошибка cd: «%1» не является каталогом.").arg(input), true};
    }
    previousDirectory_ = directory_;
    directory_ = target;
    return {previous ? directory_ : QString{}};
}

CommandResult Shell::listDirectory(const QStringList &arguments) const
{
    FlagOptions options = parseFlags(arguments, QStringLiteral("al"));
    if (!options.valid) {
        return {usageError(QStringLiteral("ls"), QStringLiteral("ls [-al] [--] [путь ...]")), true};
    }
    if (options.paths.isEmpty()) {
        options.paths.append(directory_);
    }
    const bool hidden = options.flags.contains(u'a');
    const bool details = options.flags.contains(u'l');
    QStringList sections;
    bool error = false;
    for (const QString &input : options.paths) {
        const QString path = resolvePath(input);
        const auto node = vfs_.nodes().constFind(path);
        QStringList entries;
        if (node == vfs_.nodes().cend()) {
            entries.append(QStringLiteral("Ошибка ls: путь «%1» не найден в VFS.").arg(input));
            error = true;
        } else if (!node->directory) {
            if (input.endsWith(u'/')) {
                entries.append(QStringLiteral("Ошибка ls: «%1» не является каталогом.").arg(input));
                error = true;
            } else {
                entries.append(entryText(path.mid(path.lastIndexOf(u'/') + 1), *node, details));
            }
        } else {
            if (hidden) {
                entries.append(entryText(QStringLiteral("."), *node, details));
                entries.append(entryText(QStringLiteral(".."), vfs_.nodes().value(parentPath(path)), details));
            }
            const QString prefix = path == QStringLiteral("/") ? path : path + u'/';
            for (auto it = vfs_.nodes().cbegin(); it != vfs_.nodes().cend(); ++it) {
                if (!it.key().startsWith(prefix) || it.key() == path) {
                    continue;
                }
                const QString name = it.key().mid(prefix.size());
                if (name.contains(u'/') || (!hidden && name.startsWith(u'.'))) {
                    continue;
                }
                entries.append(entryText(name, it.value(), details));
            }
        }
        const QString heading = options.paths.size() > 1 ? input + QStringLiteral(":\n") : QString{};
        sections.append(heading + entries.join(u'\n'));
    }
    return {sections.join(QStringLiteral("\n\n")), error};
}
