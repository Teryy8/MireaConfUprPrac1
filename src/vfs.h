/*
 * Объявляет VfsNode с типом элемента, содержимым и правами, а также класс Vfs.
 * Элементы хранятся в QMap: ключом служит абсолютный путь внутри VFS.
 */

#pragma once

#include <QByteArray>
#include <QMap>
#include <QString>

struct VfsNode {
    bool directory = true;
    QByteArray data;
    unsigned int permissions = 0755;
};

class Vfs {
public:
    Vfs();
    QString load(const QString &path);
    QString reset();
    QString setPermissions(const QString &path, unsigned int permissions);
    QString remove(const QString &path, bool recursive, bool emptyDirectories);
    QString summary() const;
    const QMap<QString, VfsNode> &nodes() const;

private:
    QString sourcePath_;
    QMap<QString, VfsNode> nodes_;
};
