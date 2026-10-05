#pragma once

#include "shell.h"
#include "configuration.h"

#include <QMainWindow>

class QLineEdit;
class QPlainTextEdit;

/// Графический REPL: ввод, выполнение, вывод и ожидание следующей команды.
class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);
    explicit MainWindow(const Configuration &configuration, QWidget *parent = nullptr);
    void runStartupScript();

private:
    void submitCommand();
    void displayCommand(const QString &line, const CommandResult &result);

    Configuration configuration_;
    Shell shell_;
    QLineEdit *input_;
    QPlainTextEdit *transcript_;
    QString vfsName_;
};
