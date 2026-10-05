#include "startupscript.h"

#include <QFile>
#include <QStringConverter>
#include <QTextStream>

ScriptResult runStartupScript(const QString &path, Shell &shell, const CommandDisplay &display)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {QStringLiteral("Ошибка открытия скрипта «%1»: %2").arg(path, file.errorString()), true};
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    int lineNumber = 0;
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        ++lineNumber;
        if (stream.status() != QTextStream::Ok) {
            return {QStringLiteral("Ошибка чтения скрипта «%1».").arg(path), true, false, lineNumber};
        }
        if (line.trimmed().isEmpty()) {
            continue;
        }
        const CommandResult command = shell.execute(line);
        display(line, command);
        if (command.error) {
            return {QStringLiteral("Скрипт «%1» остановлен: ошибка в строке %2.").arg(path).arg(lineNumber),
                    true, false, lineNumber};
        }
        if (command.exitRequested) {
            return {{}, false, true, lineNumber};
        }
    }
    return {QStringLiteral("Стартовый скрипт выполнен."), false, false, lineNumber};
}
