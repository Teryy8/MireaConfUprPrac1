/*
 * Объявляет настройки запуска: пути к VFS и стартовому скрипту.
 * ConfigurationResult хранит настройки, ошибку разбора или текст справки.
 */

#pragma once

#include <QStringList>

struct Configuration {
    QString vfsPath = QStringLiteral("vfs/VFS-17.csv");
    QString scriptPath;

    QString vfsName() const;
    QString debugText() const;
};

struct ConfigurationResult {
    Configuration configuration;
    QString error;
    QString information;
};

ConfigurationResult parseConfiguration(const QStringList &arguments);
