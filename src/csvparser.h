#pragma once

#include <QStringList>

struct CsvRow {
    QStringList fields;
    QString error;
};

CsvRow parseCsvRow(const QString &line);
