#include "mainwindow.h"

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QtTest>

class GuiTest : public QObject {
    Q_OBJECT

private slots:
    void interactiveDialog();
    void exitClosesWindow();
};

void GuiTest::interactiveDialog()
{
    MainWindow window;
    window.show();
    QVERIFY(window.windowTitle().contains(QStringLiteral("VFS-17")));
    auto *input = window.findChild<QLineEdit *>(QStringLiteral("commandInput"));
    auto *output = window.findChild<QPlainTextEdit *>(QStringLiteral("transcript"));
    auto *button = window.findChild<QPushButton *>(QStringLiteral("executeButton"));
    QVERIFY(input && output && button);
    QVERIFY(output->isReadOnly());

    input->setText(QStringLiteral("ls"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(input->text().isEmpty());
    QVERIFY(output->toPlainText().contains(QStringLiteral("Заглушка: ls")));

    input->setText(QStringLiteral("cd \"$HOME\""));
    QTest::mouseClick(button, Qt::LeftButton);
    QVERIFY(output->toPlainText().contains(QProcessEnvironment::systemEnvironment().value("HOME")));
    QVERIFY(output->toPlainText().contains(QStringLiteral("Заглушка: cd")));

    input->setText(QStringLiteral("unknown"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(output->toPlainText().contains(QStringLiteral("неизвестная команда")));

    input->setText(QStringLiteral("cd /one /two"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(output->toPlainText().contains(QStringLiteral("неверные аргументы cd")));

    input->setText(QStringLiteral("cd \"broken"));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(output->toPlainText().contains(QStringLiteral("незакрытая кавычка")));

    input->setText(QStringLiteral("ls -al \"my folder\""));
    QTest::keyClick(input, Qt::Key_Return);
    QVERIFY(output->toPlainText().contains(QStringLiteral("[\"-al\",\"my folder\"]")));
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

QTEST_MAIN(GuiTest)
#include "test_gui.moc"
