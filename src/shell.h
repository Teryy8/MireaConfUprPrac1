#pragma once

#include "commandparser.h"

struct CommandResult {
    QString output;
    bool error = false;
    bool exitRequested = false;
};

/// Выполняет команды этапа 1, не обращаясь к файловой системе.
class Shell {
public:
    explicit Shell(QProcessEnvironment environment = QProcessEnvironment::systemEnvironment());
    CommandResult execute(const QString &line) const;

private:
    CommandParser parser_;
};
