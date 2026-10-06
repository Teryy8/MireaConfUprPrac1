/*
 * Загружает VFS из CSV в память, проверяет пути, типы, base64 и родительские каталоги.
 * Реализует изменение прав, удаление и подсчёт элементов. Сброс VFS оставляет
 * пустой корень и очищает выбранный CSV на диске.
 */

#include "vfs.h"
#include "csvparser.h"

#include <QFile>
#include <QFileInfo>
#include <QStringConverter>

namespace {
QMap<QString, VfsNode> defaultNodes()
{
    return {{QStringLiteral("/"), VfsNode{}}};
}

bool validPath(const QString &path)
{
    if (path == QStringLiteral("/")) {
        return true;
    }
    if (!path.startsWith(u'/') || path.endsWith(u'/')) {
        return false;
    }
    for (const QChar ch : path) {
        if (ch.unicode() < 32 || ch.unicode() == 127) {
            return false;
        }
    }
    const QStringList parts = path.mid(1).split(u'/');
    for (const QString &part : parts) {
        if (part.isEmpty() || part == QStringLiteral(".") || part == QStringLiteral("..")) {
            return false;
        }
    }
    return true;
}

QString formatError(qsizetype line, const QString &message)
{
    return QStringLiteral("Неверный формат VFS, строка %1: %2.").arg(line).arg(message);
}

QString readNode(const QStringList &fields, QMap<QString, VfsNode> &nodes)
{
    if (fields.size() != 3) {
        return QStringLiteral("ожидаются три поля: path,type,data");
    }
    const QString &path = fields[0];
    if (!validPath(path)) {
        return QStringLiteral("неверный абсолютный путь «%1»").arg(path);
    }
    if (nodes.contains(path)) {
        return QStringLiteral("повторный путь «%1»").arg(path);
    }
    VfsNode node;
    if (fields[1] == QStringLiteral("dir")) {
        if (!fields[2].isEmpty()) {
            return QStringLiteral("каталог не должен содержать данные");
        }
    } else if (fields[1] == QStringLiteral("file")) {
        if (path == QStringLiteral("/")) {
            return QStringLiteral("корень должен быть каталогом");
        }
        const QByteArray encoded = fields[2].toUtf8();
        const auto decoded = QByteArray::fromBase64Encoding(encoded, QByteArray::AbortOnBase64DecodingErrors);
        if (!decoded || decoded.decoded.toBase64() != encoded) {
            return QStringLiteral("неверное содержимое base64");
        }
        node.directory = false;
        node.data = decoded.decoded;
        node.permissions = 0644;
    } else {
        return QStringLiteral("неизвестный тип «%1»").arg(fields[1]);
    }
    nodes.insert(path, node);
    return {};
}

QString readCsv(const QString &text, QMap<QString, VfsNode> &nodes)
{
    bool headerRead = false;
    const QStringList lines = text.split(u'\n');
    for (qsizetype i = 0; i < lines.size(); ++i) {
        QString line = lines[i];
        if (line.endsWith(u'\r')) {
            line.chop(1);
        }
        if (line.trimmed().isEmpty()) {
            continue;
        }
        const CsvRow row = parseCsvRow(line);
        if (!row.error.isEmpty()) {
            return formatError(i + 1, row.error);
        }
        if (!headerRead) {
            if (row.fields != QStringList{"path", "type", "data"}) {
                return formatError(i + 1, QStringLiteral("ожидается заголовок path,type,data"));
            }
            headerRead = true;
            continue;
        }
        const QString error = readNode(row.fields, nodes);
        if (!error.isEmpty()) {
            return formatError(i + 1, error);
        }
    }
    if (!nodes.contains(QStringLiteral("/"))) {
        nodes.insert(QStringLiteral("/"), VfsNode{});
    }
    for (auto it = nodes.cbegin(); it != nodes.cend(); ++it) {
        if (it.key() == QStringLiteral("/")) {
            continue;
        }
        QString parent = it.key().left(it.key().lastIndexOf(u'/'));
        if (parent.isEmpty()) {
            parent = QStringLiteral("/");
        }
        const auto directory = nodes.constFind(parent);
        if (directory == nodes.cend() || !directory->directory) {
            return QStringLiteral("Неверный формат VFS: родитель «%1» отсутствует или не является каталогом.")
                .arg(parent);
        }
    }
    return {};
}
} // namespace

Vfs::Vfs()
    : nodes_(defaultNodes())
{
}

QString Vfs::load(const QString &path)
{
    sourcePath_ = path;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QStringLiteral("Ошибка загрузки VFS «%1»: %2").arg(path, file.errorString());
    }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        return QStringLiteral("Ошибка чтения VFS «%1»: %2").arg(path, file.errorString());
    }
    QStringDecoder decoder(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
    QString text = decoder(bytes);
    if (decoder.hasError()) {
        return QStringLiteral("Неверный формат VFS: требуется кодировка UTF-8.");
    }
    if (text.startsWith(QChar::ByteOrderMark)) {
        text.remove(0, 1);
    }
    QMap<QString, VfsNode> loaded;
    const QString error = readCsv(text, loaded);
    if (!error.isEmpty()) {
        return error;
    }
    nodes_ = std::move(loaded);
    return {};
}

QString Vfs::reset()
{
    if (sourcePath_.isEmpty()) {
        return QStringLiteral("Ошибка vfs-init: путь к VFS не задан.");
    }
    QFile file(sourcePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return QStringLiteral("Ошибка vfs-init: не удалось очистить «%1»: %2").arg(sourcePath_, file.errorString());
    }
    file.close();
    nodes_ = defaultNodes();
    return {};
}

QString Vfs::summary() const
{
    int directories = 0;
    for (const VfsNode &node : nodes_) {
        if (node.directory) {
            ++directories;
        }
    }
    return QStringLiteral("Каталогов: %1, файлов: %2.").arg(directories).arg(nodes_.size() - directories);
}

const QMap<QString, VfsNode> &Vfs::nodes() const
{
    return nodes_;
}

QString Vfs::setPermissions(const QString &path, unsigned int permissions)
{
    auto node = nodes_.find(path);
    if (node == nodes_.end()) {
        return QStringLiteral("путь «%1» не найден в VFS.").arg(path);
    }
    node->permissions = permissions & 07777;
    return {};
}

QString Vfs::remove(const QString &path, bool recursive, bool emptyDirectories)
{
    const auto node = nodes_.constFind(path);
    if (node == nodes_.cend()) {
        return QStringLiteral("путь «%1» не найден в VFS.").arg(path);
    }
    if (path == QStringLiteral("/")) {
        return QStringLiteral("нельзя удалить корень VFS.");
    }
    if (node->directory && !recursive && !emptyDirectories) {
        return QStringLiteral("«%1» — каталог; используйте -r или -d.").arg(path);
    }
    const QString prefix = path + u'/';
    QStringList removed{path};
    for (auto it = nodes_.cbegin(); it != nodes_.cend(); ++it) {
        if (it.key().startsWith(prefix)) {
            if (!recursive) {
                return QStringLiteral("каталог «%1» не пуст.").arg(path);
            }
            removed.append(it.key());
        }
    }
    for (const QString &key : removed) {
        nodes_.remove(key);
    }
    return {};
}
