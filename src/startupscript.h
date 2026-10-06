/*
 * Объявляет результат выполнения стартового скрипта и функцию его запуска.
 * CommandDisplay задаёт функцию обратного вызова для отображения команд.
 */

#pragma once

#include "shell.h"

#include <functional>

struct ScriptResult {
    QString message;
    bool error = false;
    bool exitRequested = false;
    int lineNumber = 0;
};

using CommandDisplay = std::function<void(const QString &, const CommandResult &)>;

ScriptResult runStartupScript(const QString &path, Shell &shell, const CommandDisplay &display);
