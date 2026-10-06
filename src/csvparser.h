/*
 * Объявляет CsvRow и функцию разбора одной строки CSV.
 * CsvRow содержит выделенные поля или сообщение об ошибке формата.
 */

#pragma once

#include <QStringList>

struct CsvRow {
    QStringList fields;
    QString error;
};

CsvRow parseCsvRow(const QString &line);
