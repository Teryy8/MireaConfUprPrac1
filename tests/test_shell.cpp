#include "shell.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class ShellTest : public QObject {
    Q_OBJECT

private slots:
    void commands_data();
    void commands();
    void errors_data();
    void errors();
    void navigation();
    void hiddenAndMultiplePaths();
    void history();
    void csvNotModified();
    void blankInput();
    void exitCommand();
};

void ShellTest::commands_data()
{
    QTest::addColumn<QString>("line");
    QTest::addColumn<QString>("expected");
    QTest::newRow("ls") << QStringLiteral("ls")
        << QStringLiteral("-directory\n-file\nbytes.bin\ndocs\nemptydir\nmy folder");
    QTest::newRow("ls-file") << QStringLiteral("ls -l /docs/lines.txt")
        << QStringLiteral("-rw-r--r-- 39 lines.txt");
    QTest::newRow("ls-directory") << QStringLiteral("ls -l /docs/projects")
        << QStringLiteral("drwxr-xr-x 0 demo");
    QTest::newRow("ls-empty") << QStringLiteral("ls /emptydir") << QString{};
    QTest::newRow("ls-dash") << QStringLiteral("ls -- -file") << QStringLiteral("-file");
    QTest::newRow("cd") << QStringLiteral("cd") << QString{};
    QTest::newRow("cd-home") << QStringLiteral("cd \"$HOME\"") << QString{};
    QTest::newRow("cd-dash-path") << QStringLiteral("cd -- -directory") << QString{};
    QTest::newRow("uniq") << QStringLiteral("uniq /docs/lines.txt")
        << QStringLiteral("alpha\nbeta\ngamma\nalpha\n");
    QTest::newRow("uniq-count") << QStringLiteral("uniq -c /docs/lines.txt")
        << QStringLiteral("      2 alpha\n      3 beta\n      1 gamma\n      1 alpha\n");
    QTest::newRow("uniq-repeated") << QStringLiteral("uniq -d /docs/lines.txt")
        << QStringLiteral("alpha\nbeta\n");
    QTest::newRow("uniq-unique") << QStringLiteral("uniq -u /docs/lines.txt")
        << QStringLiteral("gamma\nalpha\n");
    QTest::newRow("uniq-ignore-case") << QStringLiteral("uniq -i /docs/case.txt")
        << QStringLiteral("A\nB\n");
    QTest::newRow("uniq-count-duplicates") << QStringLiteral("uniq -cd /docs/lines.txt")
        << QStringLiteral("      2 alpha\n      3 beta\n");
    QTest::newRow("uniq-both-filters") << QStringLiteral("uniq -du /docs/lines.txt") << QString{};
    QTest::newRow("uniq-empty") << QStringLiteral("uniq /docs/empty.txt") << QString{};
    QTest::newRow("uniq-blank") << QStringLiteral("uniq /docs/blank.txt") << QStringLiteral("\nx\n");
    QTest::newRow("uniq-no-newline") << QStringLiteral("uniq /docs/no-newline.txt")
        << QStringLiteral("first\nlast\n");
    QTest::newRow("tail-default") << QStringLiteral("tail /docs/long.txt")
        << QStringLiteral("3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n");
    QTest::newRow("tail-n") << QStringLiteral("tail -n 2 /docs/long.txt") << QStringLiteral("11\n12\n");
    QTest::newRow("tail-attached") << QStringLiteral("tail -n2 /docs/long.txt") << QStringLiteral("11\n12\n");
    QTest::newRow("tail-zero") << QStringLiteral("tail -n 0 /docs/long.txt") << QString{};
    QTest::newRow("tail-empty") << QStringLiteral("tail /docs/empty.txt") << QString{};
    QTest::newRow("tail-no-newline") << QStringLiteral("tail -n 1 /docs/no-newline.txt") << QStringLiteral("last");
    QTest::newRow("tail-huge-count") << QStringLiteral("tail -n 9223372036854775807 /docs/no-newline.txt")
        << QStringLiteral("first\nlast");
    QTest::newRow("tail-quoted-path") << QStringLiteral("tail -- \"/my folder/note.txt\"")
        << QStringLiteral("hello\n");
}

void ShellTest::commands()
{
    QFETCH(QString, line);
    QFETCH(QString, expected);
    QProcessEnvironment environment;
    environment.insert("HOME", "/docs");
    Shell shell(environment);
    QVERIFY(shell.loadVfs(QStringLiteral("vfs/commands.csv")).isEmpty());
    const CommandResult result = shell.execute(line);
    QVERIFY2(!result.error, qPrintable(result.output));
    QVERIFY(!result.exitRequested);
    QCOMPARE(result.output, expected);
}

void ShellTest::errors_data()
{
    QTest::addColumn<QString>("line");
    QTest::addColumn<QString>("fragment");
    QTest::newRow("unknown") << QStringLiteral("unknown") << QStringLiteral("неизвестная команда");
    QTest::newRow("case") << QStringLiteral("LS") << QStringLiteral("неизвестная команда");
    QTest::newRow("cd-too-many") << QStringLiteral("cd /a /b") << QStringLiteral("неверные аргументы cd");
    QTest::newRow("cd-option") << QStringLiteral("cd -x") << QStringLiteral("неверные аргументы cd");
    QTest::newRow("cd-empty") << QStringLiteral("cd ''") << QStringLiteral("неверные аргументы cd");
    QTest::newRow("cd-file") << QStringLiteral("cd /docs/lines.txt") << QStringLiteral("не является каталогом");
    QTest::newRow("cd-missing") << QStringLiteral("cd /missing") << QStringLiteral("не найден");
    QTest::newRow("cd-previous") << QStringLiteral("cd -") << QStringLiteral("предыдущий каталог не задан");
    QTest::newRow("ls-option") << QStringLiteral("ls -z") << QStringLiteral("неверные аргументы ls");
    QTest::newRow("ls-missing") << QStringLiteral("ls /missing") << QStringLiteral("не найден");
    QTest::newRow("ls-file-slash") << QStringLiteral("ls /docs/lines.txt/") << QStringLiteral("не является каталогом");
    QTest::newRow("uniq-missing-arg") << QStringLiteral("uniq") << QStringLiteral("неверные аргументы uniq");
    QTest::newRow("uniq-option") << QStringLiteral("uniq -z /docs/lines.txt")
                               << QStringLiteral("неверные аргументы uniq");
    QTest::newRow("uniq-too-many") << QStringLiteral("uniq /a /b") << QStringLiteral("неверные аргументы uniq");
    QTest::newRow("uniq-directory") << QStringLiteral("uniq /docs") << QStringLiteral("обычным файлом");
    QTest::newRow("uniq-binary") << QStringLiteral("uniq /bytes.bin") << QStringLiteral("текстом UTF-8");
    QTest::newRow("tail-empty") << QStringLiteral("tail") << QStringLiteral("неверные аргументы tail");
    QTest::newRow("tail-option") << QStringLiteral("tail -z /a") << QStringLiteral("неверные аргументы tail");
    QTest::newRow("tail-missing-number") << QStringLiteral("tail -n") << QStringLiteral("неверные аргументы tail");
    QTest::newRow("tail-negative") << QStringLiteral("tail -n -1 /a") << QStringLiteral("неверные аргументы tail");
    QTest::newRow("tail-overflow") << QStringLiteral("tail -n 9223372036854775808 /a")
        << QStringLiteral("неверные аргументы tail");
    QTest::newRow("tail-missing-file") << QStringLiteral("tail /missing") << QStringLiteral("не найден");
    QTest::newRow("tail-directory") << QStringLiteral("tail /docs") << QStringLiteral("обычным файлом");
    QTest::newRow("tail-binary") << QStringLiteral("tail /bytes.bin") << QStringLiteral("текстом UTF-8");
    QTest::newRow("history-negative") << QStringLiteral("history -1") << QStringLiteral("неверные аргументы history");
    QTest::newRow("history-text") << QStringLiteral("history abc") << QStringLiteral("неверные аргументы history");
    QTest::newRow("history-too-many") << QStringLiteral("history 1 2") << QStringLiteral("неверные аргументы history");
    QTest::newRow("exit-argument") << QStringLiteral("exit now") << QStringLiteral("неверные аргументы exit");
    QTest::newRow("syntax") << QStringLiteral("cd \"broken") << QStringLiteral("Ошибка синтаксиса");
}

void ShellTest::errors()
{
    QFETCH(QString, line);
    QFETCH(QString, fragment);
    Shell shell;
    QVERIFY(shell.loadVfs(QStringLiteral("vfs/commands.csv")).isEmpty());
    const CommandResult result = shell.execute(line);
    QVERIFY(result.error);
    QVERIFY(!result.exitRequested);
    QVERIFY2(result.output.contains(fragment), qPrintable(result.output));
    QCOMPARE(shell.currentDirectory(), QStringLiteral("/"));
}

void ShellTest::navigation()
{
    Shell shell;
    QVERIFY(shell.loadVfs(QStringLiteral("vfs/commands.csv")).isEmpty());
    const auto first = shell.execute(QStringLiteral("cd /docs"));
    QVERIFY(!first.error);
    QCOMPARE(first.directory, QStringLiteral("/"));
    QCOMPARE(shell.currentDirectory(), QStringLiteral("/docs"));
    QVERIFY(!shell.execute(QStringLiteral("cd projects/./demo")).error);
    QCOMPARE(shell.currentDirectory(), QStringLiteral("/docs/projects/demo"));
    QVERIFY(!shell.execute(QStringLiteral("cd ..")).error);
    QCOMPARE(shell.currentDirectory(), QStringLiteral("/docs/projects"));
    QCOMPARE(shell.execute(QStringLiteral("cd -")).output, QStringLiteral("/docs/projects/demo"));
    QVERIFY(shell.execute(QStringLiteral("cd missing")).error);
    QCOMPARE(shell.currentDirectory(), QStringLiteral("/docs/projects/demo"));
    QVERIFY(!shell.execute(QStringLiteral("cd ../../../../..")).error);
    QCOMPARE(shell.currentDirectory(), QStringLiteral("/"));
    QVERIFY(!shell.execute(QStringLiteral("cd \"my folder\"")).error);
    QCOMPARE(shell.execute(QStringLiteral("tail note.txt")).output, QStringLiteral("hello\n"));
    QVERIFY(!shell.execute(QStringLiteral("cd")).error);
    QCOMPARE(shell.currentDirectory(), QStringLiteral("/"));
}

void ShellTest::hiddenAndMultiplePaths()
{
    Shell shell;
    QVERIFY(shell.loadVfs(QStringLiteral("vfs/commands.csv")).isEmpty());
    QVERIFY(!shell.execute(QStringLiteral("ls")).output.contains(QStringLiteral(".hidden")));
    const QString hidden = shell.execute(QStringLiteral("ls -a")).output;
    QVERIFY(hidden.startsWith(QStringLiteral(".\n..\n")));
    QVERIFY(hidden.contains(QStringLiteral(".hidden")));
    const auto listing = shell.execute(QStringLiteral("ls /emptydir /missing /docs/lines.txt"));
    QVERIFY(listing.error);
    QVERIFY(listing.output.contains(QStringLiteral("lines.txt")));
    const auto result = shell.execute(QStringLiteral("tail -n1 /docs/lines.txt /docs/no-newline.txt"));
    QVERIFY(!result.error);
    QCOMPARE(result.output, QStringLiteral("==> /docs/lines.txt <==\nalpha\n\n==> /docs/no-newline.txt <==\nlast"));
    const auto partial = shell.execute(QStringLiteral("tail /missing /docs/no-newline.txt"));
    QVERIFY(partial.error);
    QVERIFY(partial.output.endsWith(QStringLiteral("first\nlast")));
}

void ShellTest::history()
{
    Shell shell;
    shell.execute(QStringLiteral("ls"));
    shell.execute(QStringLiteral("  "));
    shell.execute(QStringLiteral("unknown"));
    shell.execute(QStringLiteral("cd \"broken"));
    QCOMPARE(shell.execute(QStringLiteral("history 1")).output, QStringLiteral("4  history 1"));
    QCOMPARE(shell.execute(QStringLiteral("history")).output,
        QStringLiteral("1  ls\n2  unknown\n3  cd \"broken\n4  history 1\n5  history"));
    QVERIFY(shell.execute(QStringLiteral("history 0")).output.isEmpty());
}

void ShellTest::csvNotModified()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("copy.csv"));
    QVERIFY(QFile::copy(QStringLiteral("vfs/commands.csv"), path));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray original = file.readAll();
    file.close();
    Shell shell;
    QVERIFY(shell.loadVfs(path).isEmpty());
    for (const QString &command : QStringList{"ls -al", "cd /docs", "uniq -c lines.txt", "tail long.txt", "history"}) {
        QVERIFY2(!shell.execute(command).error, qPrintable(command));
    }
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), original);
}

void ShellTest::blankInput()
{
    const auto result = Shell().execute(QStringLiteral(" \t "));
    QVERIFY(result.output.isEmpty());
    QVERIFY(!result.error);
    QVERIFY(!result.exitRequested);
}

void ShellTest::exitCommand()
{
    const auto result = Shell().execute(QStringLiteral("exit"));
    QVERIFY(!result.error);
    QVERIFY(result.exitRequested);
}

QTEST_GUILESS_MAIN(ShellTest)
#include "test_shell.moc"
