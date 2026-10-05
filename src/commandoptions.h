#pragma once

#include <QStringList>

struct FlagOptions {
    QString flags;
    QStringList paths;
    bool valid = true;
};

FlagOptions parseFlags(const QStringList &arguments, const QString &allowed);
bool parseCount(const QString &text, qlonglong &count);
QString usageError(const QString &command, const QString &usage);
