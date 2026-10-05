#include "mainwindow.h"

#include <QLineEdit>
#include <QFile>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QLabel>
#include <QtTest>

class GuiTest : public QObject {
    Q_OBJECT

private slots:
    void interactiveDialog();
    void exitClosesWindow();
    void startupErrorAllowsInput();
    void startupExitClosesWindow();
    void vfsLoadAndReset();
    void vfsLoadErrors();
};

void GuiTest::interactiveDialog()
{
    Configuration configuration;
    configuration.vfsPath = QStringLiteral("vfs/commands.csv");
    MainWindow window(configuration);
    window.show();
    QVERIFY(window.windowTitle().contains(QStringLiteral("commands")));
    auto *input = window.findChild<QLineEdit *>(QStringLiteral("commandInput"));
    auto *output = window.findChild<QPlainTextEdit *>(QStringLiteral("transcript"));
    auto *button = window.findChild<QPushButton *>(QStringLiteral("executeButton"));
    QVERIFY(input && output && button);
    QVERIFY(output->isReadOnly());

    input->setText(QStringLiteral("ls"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(input->text().isEmpty());
    QVERIFY(output->toPlainText().contains(QStringLiteral("bytes.bin\ndocs")));

    input->setText(QStringLiteral("cd /docs"));
    QTest::mouseClick(button, Qt::LeftButton);
    QVERIFY(output->toPlainText().contains(QStringLiteral("commands:/$ cd /docs")));
    auto *prompt = window.findChild<QLabel *>(QStringLiteral("prompt"));
    QVERIFY(prompt);
    QCOMPARE(prompt->text(), QStringLiteral("commands:/docs$"));

    input->setText(QStringLiteral("unknown"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(output->toPlainText().contains(QStringLiteral("неизвестная команда")));

    input->setText(QStringLiteral("cd /one /two"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(output->toPlainText().contains(QStringLiteral("неверные аргументы cd")));

    input->setText(QStringLiteral("cd \"broken"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(output->toPlainText().contains(QStringLiteral("незакрытая кавычка")));

    input->setText(QStringLiteral("ls -al \"/my folder\""));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(output->toPlainText().contains(QStringLiteral("-rw-r--r-- 6 note.txt")));
    input->setText(QStringLiteral("tail -n 1 lines.txt"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(output->toPlainText().contains(QStringLiteral("commands:/docs$ tail -n 1 lines.txt\nalpha")));
    input->setText(QStringLiteral("history 2"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(output->toPlainText().contains(QStringLiteral("  tail -n 1 lines.txt")));
    input->setText(QStringLiteral("chmod 600 lines.txt"));
    QTest::keyClick(input, Qt::Key_Return);
    input->setText(QStringLiteral("ls -l lines.txt"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(output->toPlainText().contains(QStringLiteral("-rw------- 39 lines.txt")));
    input->setText(QStringLiteral("rm lines.txt"));
    QTest::mouseClick(button, Qt::LeftButton);
    input->setText(QStringLiteral("tail lines.txt"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(output->toPlainText().contains(QStringLiteral("файл «lines.txt» не найден в VFS")));
    QVERIFY(window.isVisible());
}

void GuiTest::exitClosesWindow()
{
    MainWindow window;
    window.show();
    auto *input = window.findChild<QLineEdit *>(QStringLiteral("commandInput"));
    QVERIFY(input);
    input->setText(QStringLiteral("exit now"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(window.isVisible());
    input->setText(QStringLiteral("exit"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(!window.isVisible());
}

void GuiTest::startupErrorAllowsInput()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    Configuration configuration;
    configuration.vfsPath = directory.filePath(QStringLiteral("my filesystem.csv"));
    QVERIFY(QFile::copy(QStringLiteral("vfs/commands.csv"), configuration.vfsPath));
    configuration.scriptPath = directory.filePath(QStringLiteral("startup.txt"));
    QFile file(configuration.scriptPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("ls /\nunknown\nls after\n");
    file.close();
    MainWindow window(configuration);
    window.show();
    window.runStartupScript();
    auto *output = window.findChild<QPlainTextEdit *>(QStringLiteral("transcript"));
    auto *input = window.findChild<QLineEdit *>(QStringLiteral("commandInput"));
    QVERIFY(output && input);
    QVERIFY(window.windowTitle().contains(QStringLiteral("my filesystem")));
    QVERIFY(output->toPlainText().contains(configuration.debugText()));
    QVERIFY(output->toPlainText().contains(QStringLiteral("my filesystem:/$ ls /")));
    QVERIFY(output->toPlainText().contains(QStringLiteral("строке 2")));
    QVERIFY(!output->toPlainText().contains(QStringLiteral("ls after")));
    QVERIFY(window.isVisible());
    input->setText(QStringLiteral("tail /docs/no-newline.txt"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(output->toPlainText().contains(QStringLiteral("first\nlast")));
}

void GuiTest::startupExitClosesWindow()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    Configuration configuration;
    configuration.scriptPath = directory.filePath(QStringLiteral("exit.txt"));
    QFile file(configuration.scriptPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("exit\n");
    file.close();
    MainWindow window(configuration);
    window.show();
    window.runStartupScript();
    QVERIFY(!window.isVisible());
}

void GuiTest::vfsLoadAndReset()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    Configuration configuration;
    configuration.vfsPath = directory.filePath(QStringLiteral("nested.csv"));
    QVERIFY(QFile::copy(QStringLiteral("vfs/nested.csv"), configuration.vfsPath));
    MainWindow window(configuration);
    window.show();
    auto *output = window.findChild<QPlainTextEdit *>(QStringLiteral("transcript"));
    auto *input = window.findChild<QLineEdit *>(QStringLiteral("commandInput"));
    QVERIFY(output && input);
    QVERIFY(output->toPlainText().contains(QStringLiteral("VFS загружена: nested")));
    QVERIFY(output->toPlainText().contains(QStringLiteral("Каталогов: 4, файлов: 2")));
    input->setText(QStringLiteral("vfs-init"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(output->toPlainText().contains(QStringLiteral("Каталогов: 1, файлов: 0")));
    QCOMPARE(QFile(configuration.vfsPath).size(), 0);
}

void GuiTest::vfsLoadErrors()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    Configuration configuration;
    configuration.vfsPath = directory.filePath(QStringLiteral("missing.csv"));
    MainWindow missing(configuration);
    auto *output = missing.findChild<QPlainTextEdit *>(QStringLiteral("transcript"));
    QVERIFY(output);
    QVERIFY(output->toPlainText().contains(QStringLiteral("Ошибка загрузки VFS")));
    QFile file(configuration.vfsPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("bad csv\n");
    file.close();
    MainWindow invalid(configuration);
    output = invalid.findChild<QPlainTextEdit *>(QStringLiteral("transcript"));
    QVERIFY(output);
    QVERIFY(output->toPlainText().contains(QStringLiteral("Неверный формат VFS")));
}

QTEST_MAIN(GuiTest)
#include "test_gui.moc"
