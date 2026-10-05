#pragma once

#include <QByteArray>
#include <QMap>
#include <QString>

struct VfsNode {
    bool directory = true;
    QByteArray data;
};

class Vfs {
public:
    Vfs();
    QString load(const QString &path);
    QString reset();
    QString summary() const;
    const QMap<QString, VfsNode> &nodes() const;

private:
    QString sourcePath_;
    QMap<QString, VfsNode> nodes_;
};
