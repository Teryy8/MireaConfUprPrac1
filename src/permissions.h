/*
 * Объявляет PermissionMode и PermissionChange для описания изменения прав.
 * Режим может быть числовым или последовательностью символьных операций.
 */

#pragma once

#include <QString>
#include <QVector>

struct PermissionChange {
    unsigned int users = 0;
    QChar operation;
    QString permissions;
};

struct PermissionMode {
    int numeric = -1;
    QVector<PermissionChange> changes;

    bool parse(const QString &text);
    unsigned int apply(unsigned int current, bool directory) const;
};
