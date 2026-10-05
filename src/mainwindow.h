#pragma once

#include "shell.h"

#include <QMainWindow>

class QLineEdit;
class QPlainTextEdit;

/// Графический REPL: ввод, выполнение, вывод и ожидание следующей команды.
class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void submitCommand();

    Shell shell_;
    QLineEdit *input_;
    QPlainTextEdit *transcript_;
    const QString vfsName_ = QStringLiteral("VFS-17");
};
