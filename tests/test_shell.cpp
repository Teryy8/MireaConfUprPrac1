#include "shell.h"

#include <QtTest>

class ShellTest : public QObject {
    Q_OBJECT

private slots:
    void commands_data();
    void commands();
    void errors_data();
    void errors();
    void blankInput();
    void exitCommand();
};

void ShellTest::commands_data()
{
    QTest::addColumn<QString>("line");
    QTest::addColumn<QString>("expected");
    QTest::newRow("ls") << QStringLiteral("ls") << QStringLiteral("Заглушка: ls\nАргументы: []");
    QTest::newRow("ls-options") << QStringLiteral("ls -al /docs")
                               << QStringLiteral("Заглушка: ls\nАргументы: [\"-al\",\"/docs\"]");
    QTest::newRow("ls-paths") << QStringLiteral("ls /a /b")
                             << QStringLiteral("Заглушка: ls\nАргументы: [\"/a\",\"/b\"]");
    QTest::newRow("cd") << QStringLiteral("cd") << QStringLiteral("Заглушка: cd\nАргументы: []");
    QTest::newRow("cd-home") << QStringLiteral("cd \"$HOME\"")
                            << QStringLiteral("Заглушка: cd\nАргументы: [\"/home/test\"]");
    QTest::newRow("cd-dash-path") << QStringLiteral("cd -- -folder")
                                 << QStringLiteral("Заглушка: cd\nАргументы: [\"--\",\"-folder\"]");
}

void ShellTest::commands()
{
    QFETCH(QString, line);
    QFETCH(QString, expected);
    QProcessEnvironment environment;
    environment.insert("HOME", "/home/test");
    const CommandResult result = Shell(environment).execute(line);
    QVERIFY(!result.error);
    QVERIFY(!result.exitRequested);
    QCOMPARE(result.output, expected);
}

void ShellTest::errors_data()
{
    QTest::addColumn<QString>("line");
    QTest::addColumn<QString>("fragment");
    QTest::newRow("unknown") << QStringLiteral("unknown") << QStringLiteral("неизвестная команда");
    QTest::newRow("case-sensitive") << QStringLiteral("LS") << QStringLiteral("неизвестная команда");
    QTest::newRow("cd-too-many") << QStringLiteral("cd /a /b") << QStringLiteral("неверные аргументы cd");
    QTest::newRow("cd-option") << QStringLiteral("cd -x") << QStringLiteral("неверные аргументы cd");
    QTest::newRow("ls-option") << QStringLiteral("ls -z") << QStringLiteral("неверные аргументы ls");
    QTest::newRow("empty-path") << QStringLiteral("cd ''") << QStringLiteral("неверные аргументы cd");
    QTest::newRow("exit-argument") << QStringLiteral("exit now") << QStringLiteral("неверные аргументы exit");
    QTest::newRow("syntax") << QStringLiteral("cd \"broken") << QStringLiteral("Ошибка синтаксиса");
}

void ShellTest::errors()
{
    QFETCH(QString, line);
    QFETCH(QString, fragment);
    const CommandResult result = Shell(QProcessEnvironment{}).execute(line);
    QVERIFY(result.error);
    QVERIFY(!result.exitRequested);
    QVERIFY2(result.output.contains(fragment), qPrintable(result.output));
}

void ShellTest::blankInput()
{
    const CommandResult result = Shell().execute(QStringLiteral("  \t "));
    QVERIFY(result.output.isEmpty());
    QVERIFY(!result.error);
    QVERIFY(!result.exitRequested);
}

void ShellTest::exitCommand()
{
    const CommandResult result = Shell().execute(QStringLiteral("exit"));
    QVERIFY(!result.error);
    QVERIFY(result.exitRequested);
}

QTEST_GUILESS_MAIN(ShellTest)
#include "test_shell.moc"
