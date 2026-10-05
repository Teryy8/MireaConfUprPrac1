#pragma once

#include <QProcessEnvironment>
#include <QStringList>

struct ParseResult {
    QStringList words;
    QString error;
};

class CommandParser {
public:
    explicit CommandParser(QProcessEnvironment environment = QProcessEnvironment::systemEnvironment());
    ParseResult parse(const QString &line) const;

private:
    QProcessEnvironment environment_;
};
