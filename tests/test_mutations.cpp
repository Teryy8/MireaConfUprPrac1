#include "shell.h"
#include "startupscript.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class MutationsTest : public QObject {
    Q_OBJECT

private slots:
    void chmodModes_data();
    void chmodModes();
    void recursivePermissions();
    void chmodErrors_data();
    void chmodErrors();
    void removeFilesAndDirectories();
    void removeErrors_data();
    void removeErrors();
    void partialResults();
    void navigationAfterRemoval();
    void onlyMemoryChanges();
    void errorScripts();
};

void MutationsTest::chmodModes_data()
{
    QTest::addColumn<QString>("mode");
    QTest::addColumn<unsigned int>("expected");
    QTest::newRow("numeric") << QStringLiteral("600") << 0600u;
    QTest::newRow("leading-zero") << QStringLiteral("0750") << 0750u;
    QTest::newRow("zero") << QStringLiteral("0") << 0u;
    QTest::newRow("special") << QStringLiteral("6755") << 06755u;
    QTest::newRow("add") << QStringLiteral("u+x") << 0744u;
    QTest::newRow("remove") << QStringLiteral("go-r") << 0600u;
    QTest::newRow("set") << QStringLiteral("a=r") << 0444u;
    QTest::newRow("empty-set") << QStringLiteral("u=") << 0044u;
    QTest::newRow("commas") << QStringLiteral("u=rwx,g=rx,o=") << 0750u;
    QTest::newRow("chained") << QStringLiteral("u+x-w") << 0544u;
    QTest::newRow("implicit-all") << QStringLiteral("+x") << 0755u;
    QTest::newRow("minus-mode") << QStringLiteral("-w") << 0444u;
    QTest::newRow("copy") << QStringLiteral("g=u,o=g") << 0666u;
    QTest::newRow("conditional-execute") << QStringLiteral("a+X") << 0644u;
    QTest::newRow("setuid") << QStringLiteral("u+s") << 04644u;
    QTest::newRow("setgid") << QStringLiteral("g+s") << 02644u;
    QTest::newRow("sticky") << QStringLiteral("o+t") << 01644u;
}

void MutationsTest::chmodModes()
{
    QFETCH(QString, mode);
    QFETCH(unsigned int, expected);
    Shell shell;
    QVERIFY(shell.loadVfs(QStringLiteral("vfs/commands.csv")).isEmpty());
    const CommandResult result = shell.execute(QStringLiteral("chmod %1 /docs/lines.txt").arg(mode));
    QVERIFY2(!result.error, qPrintable(result.output));
    QVERIFY(result.output.isEmpty());
    QCOMPARE(shell.vfs().nodes().value(QStringLiteral("/docs/lines.txt")).permissions, expected);
    QCOMPARE(shell.vfs().nodes().value(QStringLiteral("/docs/case.txt")).permissions, 0644u);
}

void MutationsTest::recursivePermissions()
{
    Shell shell;
    QVERIFY(shell.loadVfs(QStringLiteral("vfs/commands.csv")).isEmpty());
    QVERIFY(!shell.execute(QStringLiteral("chmod -R a=rwX /docs")).error);
    QCOMPARE(shell.vfs().nodes().value(QStringLiteral("/docs/projects/demo")).permissions, 0777u);
    QCOMPARE(shell.vfs().nodes().value(QStringLiteral("/docs/projects/demo/report.txt")).permissions, 0666u);
    QCOMPARE(shell.vfs().nodes().value(QStringLiteral("/my folder/note.txt")).permissions, 0644u);
    QVERIFY(!shell.execute(QStringLiteral("chmod -v 4755 /docs/lines.txt")).error);
    QCOMPARE(shell.execute(QStringLiteral("ls -l /docs/lines.txt")).output,
             QStringLiteral("-rwsr-xr-x 39 lines.txt"));
    QVERIFY(!shell.execute(QStringLiteral("chmod u-x /docs/lines.txt")).error);
    QCOMPARE(shell.execute(QStringLiteral("ls -l /docs/lines.txt")).output,
             QStringLiteral("-rwSr-xr-x 39 lines.txt"));
    QVERIFY(!shell.execute(QStringLiteral("chmod -- 640 -- -file")).error);
    QCOMPARE(shell.vfs().nodes().value(QStringLiteral("/-file")).permissions, 0640u);
    QVERIFY(!shell.execute(QStringLiteral("chmod -Rv 1777 /emptydir")).error);
    QVERIFY(shell.execute(QStringLiteral("ls -l /")).output.contains(QStringLiteral("drwxrwxrwt 0 emptydir")));
    QVERIFY(!shell.execute(QStringLiteral("chmod -R 700 /")).error);
    for (const VfsNode &node : shell.vfs().nodes()) QCOMPARE(node.permissions, 0700u);
}

void MutationsTest::chmodErrors_data()
{
    QTest::addColumn<QString>("command");
    for (const QString &command : QStringList{
             "chmod", "chmod 644", "chmod -z 644 /docs", "chmod 888 /docs", "chmod 10000 /docs",
             "chmod u+z /docs", "chmod u /docs", "chmod u+r, /docs", "chmod u=ur /docs",
             "chmod '' /docs", "chmod 644 ''", "chmod 644 /missing", "chmod 644 /docs/lines.txt/"}) {
        QTest::newRow(qPrintable(command)) << command;
    }
}

void MutationsTest::chmodErrors()
{
    QFETCH(QString, command);
    Shell shell;
    QVERIFY(shell.loadVfs(QStringLiteral("vfs/commands.csv")).isEmpty());
    const auto result = shell.execute(command);
    QVERIFY2(result.error, qPrintable(command));
    QVERIFY(!result.exitRequested);
    QCOMPARE(shell.vfs().nodes().value(QStringLiteral("/docs")).permissions, 0755u);
    QCOMPARE(shell.vfs().nodes().value(QStringLiteral("/docs/lines.txt")).permissions, 0644u);
}

void MutationsTest::removeFilesAndDirectories()
{
    Shell shell;
    QVERIFY(shell.loadVfs(QStringLiteral("vfs/commands.csv")).isEmpty());
    QVERIFY(!shell.execute(QStringLiteral("cd /docs")).error);
    QVERIFY(!shell.execute(QStringLiteral("rm -v lines.txt empty.txt")).error);
    QVERIFY(!shell.vfs().nodes().contains(QStringLiteral("/docs/lines.txt")));
    QVERIFY(shell.execute(QStringLiteral("tail lines.txt")).error);
    QVERIFY(!shell.execute(QStringLiteral("rm -R projects")).error);
    QVERIFY(!shell.vfs().nodes().contains(QStringLiteral("/docs/projects/demo/report.txt")));
    QVERIFY(shell.vfs().nodes().contains(QStringLiteral("/docs/case.txt")));
    QVERIFY(!shell.execute(QStringLiteral("rm -d /emptydir")).error);
    QVERIFY(!shell.execute(QStringLiteral("rm -- /-file")).error);
    QVERIFY(!shell.execute(QStringLiteral("rm -rv -- /-directory")).error);
    QVERIFY(!shell.execute(QStringLiteral("rm \"/my folder/note.txt\" /.hidden /bytes.bin")).error);
    QVERIFY(!shell.execute(QStringLiteral("rm -d \"/my folder\"")).error);
    QVERIFY(!shell.execute(QStringLiteral("rm -f /missing")).error);
    QVERIFY(!shell.execute(QStringLiteral("rm -f")).error);
    QVERIFY(!shell.execute(QStringLiteral("rm -rf /missing")).error);
    QCOMPARE(shell.currentDirectory(), QStringLiteral("/docs"));
}

void MutationsTest::removeErrors_data()
{
    QTest::addColumn<QString>("command");
    for (const QString &command : QStringList{
             "rm", "rm -z /docs", "rm ''", "rm /missing", "rm /docs", "rm -f /docs",
             "rm -d /docs", "rm -rf /", "rm -rf .", "rm -rf ..", "rm -rf /docs/.",
             "rm -rf /docs/..", "rm -rf /./", "rm /docs/lines.txt/", "rm -f /docs/lines.txt/"}) {
        QTest::newRow(qPrintable(command)) << command;
    }
}

void MutationsTest::removeErrors()
{
    QFETCH(QString, command);
    Shell shell;
    QVERIFY(shell.loadVfs(QStringLiteral("vfs/commands.csv")).isEmpty());
    const qsizetype count = shell.vfs().nodes().size();
    QVERIFY2(shell.execute(command).error, qPrintable(command));
    QCOMPARE(shell.vfs().nodes().size(), count);
    QVERIFY(shell.vfs().nodes().contains(QStringLiteral("/docs/lines.txt")));
}

void MutationsTest::partialResults()
{
    Shell shell;
    QVERIFY(shell.loadVfs(QStringLiteral("vfs/commands.csv")).isEmpty());
    const auto chmod = shell.execute(QStringLiteral("chmod -v 600 /missing /docs/lines.txt /docs/case.txt"));
    QVERIFY(chmod.error);
    QVERIFY(chmod.output.contains(QStringLiteral("0644 -> 0600")));
    QCOMPARE(shell.vfs().nodes().value(QStringLiteral("/docs/case.txt")).permissions, 0600u);
    const auto rm = shell.execute(QStringLiteral("rm -v /missing /docs/lines.txt /docs/case.txt"));
    QVERIFY(rm.error);
    QVERIFY(rm.output.contains(QStringLiteral("rm: удалён")));
    QVERIFY(!shell.vfs().nodes().contains(QStringLiteral("/docs/case.txt")));
}

void MutationsTest::navigationAfterRemoval()
{
    Shell shell;
    QVERIFY(shell.loadVfs(QStringLiteral("vfs/commands.csv")).isEmpty());
    QVERIFY(!shell.execute(QStringLiteral("cd /docs/projects/demo")).error);
    QVERIFY(shell.execute(QStringLiteral("rm -rf /docs")).error);
    QVERIFY(shell.execute(QStringLiteral("rm -rf /docs/projects/demo")).error);
    QCOMPARE(shell.currentDirectory(), QStringLiteral("/docs/projects/demo"));
    QVERIFY(!shell.execute(QStringLiteral("cd /")).error);
    QVERIFY(!shell.execute(QStringLiteral("rm -r /docs/projects")).error);
    QVERIFY(shell.execute(QStringLiteral("cd -")).error);
    QCOMPARE(shell.currentDirectory(), QStringLiteral("/"));
    QVERIFY(shell.execute(QStringLiteral("cd /docs/projects/demo")).error);
}

void MutationsTest::onlyMemoryChanges()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("vfs.csv"));
    QVERIFY(QFile::copy(QStringLiteral("vfs/commands.csv"), path));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray original = file.readAll();
    const QFileDevice::Permissions hostPermissions = file.permissions();
    file.close();
    Shell shell;
    QVERIFY(shell.loadVfs(path).isEmpty());
    QVERIFY(!shell.execute(QStringLiteral("chmod -R 000 /docs")).error);
    QVERIFY(!shell.execute(QStringLiteral("rm -rf /docs")).error);
    QVERIFY(shell.execute(QStringLiteral("chmod 000 '%1'").arg(path)).error);
    QVERIFY(!shell.execute(QStringLiteral("rm -f '%1'").arg(path)).error);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), original);
    QCOMPARE(file.permissions(), hostPermissions);
    file.close();
    QVERIFY(shell.loadVfs(path).isEmpty());
    QCOMPARE(shell.vfs().nodes().value(QStringLiteral("/docs/lines.txt")).permissions, 0644u);
    QVERIFY(shell.vfs().nodes().contains(QStringLiteral("/docs/projects/demo/report.txt")));
}

void MutationsTest::errorScripts()
{
    const QDir directory(QStringLiteral("examples/stage5-errors"));
    const QStringList scripts = directory.entryList({QStringLiteral("*.txt")}, QDir::Files);
    QVERIFY(!scripts.isEmpty());
    for (const QString &script : scripts) {
        Shell shell;
        QVERIFY(shell.loadVfs(QStringLiteral("vfs/commands.csv")).isEmpty());
        QStringList executed;
        const ScriptResult result = runStartupScript(directory.filePath(script), shell,
            [&executed](const QString &line, const CommandResult &) { executed.append(line); });
        QVERIFY2(result.error, qPrintable(script));
        QCOMPARE(result.lineNumber, 2);
        QCOMPARE(executed.size(), 2);
        QVERIFY(shell.vfs().nodes().contains(QStringLiteral("/docs/lines.txt")));
        QVERIFY(shell.vfs().nodes().contains(QStringLiteral("/docs/projects/demo/report.txt")));
    }
}

QTEST_GUILESS_MAIN(MutationsTest)
#include "test_mutations.moc"
