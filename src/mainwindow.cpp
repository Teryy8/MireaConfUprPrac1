#include "mainwindow.h"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , input_(new QLineEdit(this))
    , transcript_(new QPlainTextEdit(this))
{
    setWindowTitle(QStringLiteral("Эмулятор оболочки — %1").arg(vfsName_));
    resize(860, 580);
    setMinimumSize(580, 360);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    auto *heading = new QLabel(QStringLiteral("Вариант 17 · Этап 1 · REPL"), central);
    layout->addWidget(heading);

    transcript_->setObjectName(QStringLiteral("transcript"));
    transcript_->setAccessibleName(QStringLiteral("Диалог с эмулятором"));
    transcript_->setReadOnly(true);
    transcript_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    transcript_->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    transcript_->appendPlainText(QStringLiteral("Эмулятор оболочки. VFS: %1").arg(vfsName_));
    transcript_->appendPlainText(QStringLiteral("Команды: ls, cd, exit. ls и cd — заглушки этапа 1."));
    transcript_->appendPlainText(QStringLiteral("Переменные ОС: $HOME, ${HOME}. Ввод команды — Enter."));
    layout->addWidget(transcript_, 1);

    auto *commandRow = new QHBoxLayout;
    commandRow->addWidget(new QLabel(vfsName_ + QStringLiteral(":/$"), central));
    input_->setObjectName(QStringLiteral("commandInput"));
    input_->setAccessibleName(QStringLiteral("Команда эмулятора"));
    input_->setFont(transcript_->font());
    input_->setPlaceholderText(QStringLiteral("Например: cd \"$HOME\""));
    input_->setClearButtonEnabled(true);
    commandRow->addWidget(input_, 1);
    auto *execute = new QPushButton(QStringLiteral("Выполнить"), central);
    execute->setObjectName(QStringLiteral("executeButton"));
    commandRow->addWidget(execute);
    layout->addLayout(commandRow);
    setCentralWidget(central);

    statusBar()->showMessage(QStringLiteral("Готов к вводу"));
    connect(input_, &QLineEdit::returnPressed, this, &MainWindow::submitCommand);
    connect(execute, &QPushButton::clicked, this, &MainWindow::submitCommand);
    input_->setFocus();
}

void MainWindow::submitCommand()
{
    const QString line = input_->text();
    input_->clear();
    if (line.trimmed().isEmpty()) {
        input_->setFocus();
        return;
    }
    transcript_->appendPlainText(vfsName_ + QStringLiteral(":/$ ") + line);
    const CommandResult result = shell_.execute(line);
    if (!result.output.isEmpty()) {
        transcript_->appendPlainText(result.output);
    }
    statusBar()->showMessage(result.error ? QStringLiteral("Ошибка команды") : QStringLiteral("Готов к вводу"));
    if (result.exitRequested) {
        close();
        return;
    }
    input_->setFocus();
}
