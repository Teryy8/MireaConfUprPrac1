#pragma once

#include <QProcessEnvironment>
#include <QStringList>

struct ParseResult {
    QStringList words;
    QString error;
};

/// Разбирает строку, раскрывая переменные из окружения процесса.
class CommandParser {
public:
    explicit CommandParser(QProcessEnvironment environment = QProcessEnvironment::systemEnvironment());
    ParseResult parse(const QString &line) const;

private:
    QProcessEnvironment environment_;
};
