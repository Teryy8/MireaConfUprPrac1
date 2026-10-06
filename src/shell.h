/*
 * Объявляет основной класс Shell и результат команды CommandResult.
 * Shell хранит VFS, парсер, текущий и предыдущий каталоги, историю команд
 * и обработчики поддерживаемых команд.
 */

#pragma once

#include "commandparser.h"
#include "vfs.h"

struct CommandResult {
    QString output;
    bool error = false;
    bool exitRequested = false;
    QString directory = QStringLiteral("/");
};

class Shell {
public:
    explicit Shell(QProcessEnvironment environment = QProcessEnvironment::systemEnvironment());
    CommandResult execute(const QString &line);
    QString loadVfs(const QString &path);
    const Vfs &vfs() const;
    QString currentDirectory() const;

private:
    CommandParser parser_;
    Vfs vfs_;
    QString directory_ = QStringLiteral("/");
    QString previousDirectory_;
    QStringList history_;

    CommandResult executeWords(const QStringList &words);
    CommandResult listDirectory(const QStringList &arguments) const;
    CommandResult changeDirectory(const QStringList &arguments);
    CommandResult uniq(const QStringList &arguments) const;
    CommandResult tail(const QStringList &arguments) const;
    CommandResult history(const QStringList &arguments) const;
    CommandResult chmod(const QStringList &arguments);
    CommandResult remove(const QStringList &arguments);
    QString resolvePath(const QString &path) const;
    QString readTextFile(const QString &path, QString &text) const;
};
