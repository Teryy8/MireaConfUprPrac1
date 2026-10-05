#include "configuration.h"

#include <QtTest>

class ConfigurationTest : public QObject {
    Q_OBJECT

private slots:
    void defaults();
    void parameters();
    void invalid_data();
    void invalid();
    void helpAndVersion();
};

void ConfigurationTest::defaults()
{
    const auto result = parseConfiguration({"emulator"});
    QVERIFY(result.error.isEmpty());
    QCOMPARE(result.configuration.vfsPath, QStringLiteral("VFS-17.csv"));
    QVERIFY(result.configuration.scriptPath.isEmpty());
    QCOMPARE(result.configuration.vfsName(), QStringLiteral("VFS-17"));
    QVERIFY(result.configuration.debugText().contains(QStringLiteral("не задан")));
}

void ConfigurationTest::parameters()
{
    const auto result = parseConfiguration({"emulator", "--vfs", "/tmp/my vfs.csv", "--script", "my script.txt"});
    QVERIFY(result.error.isEmpty());
    QCOMPARE(result.configuration.vfsPath, QStringLiteral("/tmp/my vfs.csv"));
    QCOMPARE(result.configuration.scriptPath, QStringLiteral("my script.txt"));
    QCOMPARE(result.configuration.vfsName(), QStringLiteral("my vfs"));
    QVERIFY(result.configuration.debugText().contains(QStringLiteral("my script.txt")));

    const auto onlyVfs = parseConfiguration({"emulator", "--vfs=other.csv"});
    QCOMPARE(onlyVfs.configuration.vfsPath, QStringLiteral("other.csv"));
    QVERIFY(onlyVfs.configuration.scriptPath.isEmpty());
    const auto onlyScript = parseConfiguration({"emulator", "--script=commands.txt"});
    QCOMPARE(onlyScript.configuration.vfsPath, QStringLiteral("VFS-17.csv"));
    QCOMPARE(onlyScript.configuration.scriptPath, QStringLiteral("commands.txt"));
}

void ConfigurationTest::invalid_data()
{
    QTest::addColumn<QStringList>("arguments");
    QTest::newRow("unknown") << QStringList{"emulator", "--unknown"};
    QTest::newRow("missing-vfs") << QStringList{"emulator", "--vfs"};
    QTest::newRow("missing-script") << QStringList{"emulator", "--script"};
    QTest::newRow("empty-vfs") << QStringList{"emulator", "--vfs", ""};
    QTest::newRow("empty-script") << QStringList{"emulator", "--script", ""};
    QTest::newRow("blank-path") << QStringList{"emulator", "--vfs", "  "};
    QTest::newRow("positional") << QStringList{"emulator", "extra"};
}

void ConfigurationTest::invalid()
{
    QFETCH(QStringList, arguments);
    QVERIFY(!parseConfiguration(arguments).error.isEmpty());
}

void ConfigurationTest::helpAndVersion()
{
    const auto help = parseConfiguration({"emulator", "--help"});
    QVERIFY(help.error.isEmpty());
    QVERIFY(help.information.contains(QStringLiteral("--vfs")));
    QVERIFY(help.information.contains(QStringLiteral("--script")));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.2.0"));
    QCOMPARE(parseConfiguration({"emulator", "--version"}).information, QStringLiteral("0.2.0"));
}

QTEST_GUILESS_MAIN(ConfigurationTest)
#include "test_configuration.moc"
