#include "startupscript.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class StartupTest : public QObject {
    Q_OBJECT

private slots:
    void successfulScript();
    void firstError_data();
    void firstError();
    void exitStopsScript();
    void missingFile();
    void vfsInitInScript();
};

void StartupTest::successfulScript()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("my script.txt"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("\nls -al \"my folder\"\r\ncd \"$HOME\"\n");
    file.close();
    QProcessEnvironment environment;
    environment.insert("HOME", "/home/test");
    QStringList inputs;
    QStringList outputs;
    Shell shell(environment);
    const auto result = runStartupScript(path, shell,
        [&](const QString &line, const CommandResult &command) {
            inputs.append(line);
            outputs.append(command.output);
        });
    QVERIFY(!result.error);
    QVERIFY(!result.exitRequested);
    QCOMPARE(result.lineNumber, 3);
    QCOMPARE(inputs.size(), 2);
    QVERIFY(outputs[0].contains(QStringLiteral("my folder")));
    QVERIFY(outputs[1].contains(QStringLiteral("/home/test")));
}

void StartupTest::firstError_data()
{
    QTest::addColumn<QByteArray>("badLine");
    QTest::newRow("unknown-command") << QByteArray("unknown");
    QTest::newRow("invalid-arguments") << QByteArray("cd /one /two");
    QTest::newRow("syntax-error") << QByteArray("cd \"broken");
}

void StartupTest::firstError()
{
    QFETCH(QByteArray, badLine);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("error.txt"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("ls before\n\n" + badLine + "\nls after\n");
    file.close();
    QStringList inputs;
    Shell shell;
    const auto result = runStartupScript(path, shell,
        [&](const QString &line, const CommandResult &) { inputs.append(line); });
    QVERIFY(result.error);
    QVERIFY(!result.exitRequested);
    QCOMPARE(result.lineNumber, 3);
    QCOMPARE(inputs.size(), 2);
    QVERIFY(result.message.contains(QStringLiteral("строке 3")));
    QVERIFY(!inputs.contains(QStringLiteral("ls after")));
}

void StartupTest::exitStopsScript()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("exit.txt"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("exit\nunknown\n");
    file.close();
    int displayed = 0;
    Shell shell;
    const auto result = runStartupScript(path, shell,
        [&](const QString &, const CommandResult &) { ++displayed; });
    QVERIFY(result.exitRequested);
    QVERIFY(!result.error);
    QCOMPARE(displayed, 1);
}

void StartupTest::missingFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    bool displayed = false;
    Shell shell;
    const auto result = runStartupScript(directory.filePath("missing.txt"), shell,
        [&](const QString &, const CommandResult &) { displayed = true; });
    QVERIFY(result.error);
    QVERIFY(!displayed);
    QVERIFY(result.message.contains(QStringLiteral("Ошибка открытия")));
}

void StartupTest::vfsInitInScript()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString vfsPath = directory.filePath(QStringLiteral("copy.csv"));
    QVERIFY(QFile::copy(QStringLiteral("vfs/nested.csv"), vfsPath));
    Shell shell;
    QVERIFY(shell.loadVfs(vfsPath).isEmpty());
    const QString path = directory.filePath(QStringLiteral("init.txt"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("vfs-init\nls /\nexit\nunknown\n");
    file.close();
    QStringList inputs;
    const auto result = runStartupScript(path, shell,
        [&](const QString &line, const CommandResult &) { inputs.append(line); });
    QVERIFY(!result.error);
    QVERIFY(result.exitRequested);
    QCOMPARE(inputs.size(), 3);
    QCOMPARE(shell.vfs().nodes().size(), 1);
    QCOMPARE(QFile(vfsPath).size(), 0);
}

QTEST_GUILESS_MAIN(StartupTest)
#include "test_startup.moc"
