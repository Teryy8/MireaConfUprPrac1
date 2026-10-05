#pragma once

#include "commandparser.h"
#include "vfs.h"

struct CommandResult {
    QString output;
    bool error = false;
    bool exitRequested = false;
};

class Shell {
public:
    explicit Shell(QProcessEnvironment environment = QProcessEnvironment::systemEnvironment());
    CommandResult execute(const QString &line);
    QString loadVfs(const QString &path);
    const Vfs &vfs() const;

private:
    CommandParser parser_;
    Vfs vfs_;
};
