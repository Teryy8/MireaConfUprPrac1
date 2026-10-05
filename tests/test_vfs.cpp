#include "csvparser.h"
#include "shell.h"
#include "vfs.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

namespace {
bool writeFile(const QString &path, const QByteArray &data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
}
} // namespace

class VfsTest : public QObject {
    Q_OBJECT

private slots:
    void csvQuotes();
    void fixtures_data();
    void fixtures();
    void binaryAndQuotedPath();
    void wrongFormat_data();
    void wrongFormat();
    void emptyAndBom();
    void parentOrder();
    void missingFile();
    void resetAndReload();
    void resetFailure();
    void initCommand();
};

void VfsTest::csvQuotes()
{
    const auto row = parseCsvRow(QStringLiteral("\"/name, \"\"quoted\"\".txt\",file,"));
    QVERIFY(row.error.isEmpty());
    QCOMPARE(row.fields, (QStringList{"/name, \"quoted\".txt", "file", ""}));
    QVERIFY(!parseCsvRow(QStringLiteral("a,\"broken")).error.isEmpty());
    QVERIFY(!parseCsvRow(QStringLiteral("a,b\"c,d")).error.isEmpty());
    QVERIFY(!parseCsvRow(QStringLiteral("a,\"b\"tail,d")).error.isEmpty());
}

void VfsTest::fixtures_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<int>("directories");
    QTest::addColumn<int>("files");
    QTest::newRow("minimal") << QStringLiteral("vfs/minimal.csv") << 1 << 0;
    QTest::newRow("files") << QStringLiteral("vfs/files.csv") << 1 << 4;
    QTest::newRow("nested") << QStringLiteral("vfs/nested.csv") << 4 << 2;
}

void VfsTest::fixtures()
{
    QFETCH(QString, path);
    QFETCH(int, directories);
    QFETCH(int, files);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString copy = directory.filePath(QStringLiteral("source.csv"));
    const QByteArray original = readFile(path);
    QVERIFY(!original.isEmpty());
    QVERIFY(writeFile(copy, original));
    Vfs vfs;
    const QString error = vfs.load(copy);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    int actualDirectories = 0;
    for (const VfsNode &node : vfs.nodes()) {
        actualDirectories += node.directory ? 1 : 0;
    }
    QCOMPARE(actualDirectories, directories);
    QCOMPARE(vfs.nodes().size() - actualDirectories, files);
    QCOMPARE(readFile(copy), original);
    QCOMPARE(QDir(directory.path()).entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot),
             (QStringList{"source.csv"}));
    if (files == 2) {
        QVERIFY(vfs.nodes().contains(QStringLiteral("/docs/projects/demo/readme.txt")));
        QCOMPARE(vfs.nodes().value("/docs/projects/demo/readme.txt").data, QByteArray("Nested VFS\n"));
    }
}

void VfsTest::binaryAndQuotedPath()
{
    Vfs vfs;
    QVERIFY(vfs.load(QStringLiteral("vfs/files.csv")).isEmpty());
    QVERIFY(vfs.nodes().contains(QStringLiteral("/name, with comma.txt")));
    QCOMPARE(vfs.nodes().value("/name, with comma.txt").data, QByteArray("comma\n"));
    QVERIFY(!vfs.nodes().value("/bytes.bin").directory);
    QCOMPARE(vfs.nodes().value("/bytes.bin").data, QByteArray::fromHex("00ff10"));
    QCOMPARE(vfs.nodes().value("/notes.txt").data, QStringLiteral("Заметки\n").toUtf8());
}

void VfsTest::wrongFormat_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::newRow("header") << QByteArray("name,type,data\n");
    QTest::newRow("field-count") << QByteArray("path,type,data\n/x,file\n");
    QTest::newRow("extra-field") << QByteArray("path,type,data\n/x,file,,extra\n");
    QTest::newRow("unknown-type") << QByteArray("path,type,data\n/x,link,\n");
    QTest::newRow("root-file") << QByteArray("path,type,data\n/,file,\n");
    QTest::newRow("directory-data") << QByteArray("path,type,data\n/x,dir,SGk=\n");
    QTest::newRow("duplicate") << QByteArray("path,type,data\n/x,file,\n/x,file,\n");
    QTest::newRow("duplicate-root") << QByteArray("path,type,data\n/,dir,\n/,dir,\n");
    QTest::newRow("missing-parent") << QByteArray("path,type,data\n/a/b,file,\n");
    QTest::newRow("file-parent") << QByteArray("path,type,data\n/a,file,\n/a/b,file,\n");
    QTest::newRow("relative-path") << QByteArray("path,type,data\nx,file,\n");
    QTest::newRow("empty-segment") << QByteArray("path,type,data\n/a//b,file,\n");
    QTest::newRow("dot-segment") << QByteArray("path,type,data\n/./x,file,\n");
    QTest::newRow("parent-segment") << QByteArray("path,type,data\n/../x,file,\n");
    QTest::newRow("trailing-slash") << QByteArray("path,type,data\n/x/,dir,\n");
    QTest::newRow("control-character") << QByteArray("path,type,data\n/x\t,file,\n");
    QTest::newRow("base64") << QByteArray("path,type,data\n/x,file,invalid!\n");
    QTest::newRow("padding") << QByteArray("path,type,data\n/x,file,YQ===\n");
    QTest::newRow("missing-padding") << QByteArray("path,type,data\n/x,file,YQ\n");
    QTest::newRow("noncanonical-base64") << QByteArray("path,type,data\n/x,file,YR==\n");
    QTest::newRow("quotes") << QByteArray("path,type,data\n\"/x,file,\n");
    QTest::newRow("utf8") << QByteArray("path,type,data\n\xff");
    QTest::newRow("incomplete-utf8") << QByteArray("path,type,data\n\xd0");
}

void VfsTest::wrongFormat()
{
    QFETCH(QByteArray, data);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("bad.csv"));
    QVERIFY(writeFile(path, data));
    Vfs vfs;
    QVERIFY(vfs.load(QStringLiteral("vfs/files.csv")).isEmpty());
    QVERIFY(!vfs.load(path).isEmpty());
    QCOMPARE(vfs.nodes().size(), 5);
    QCOMPARE(vfs.nodes().value("/hello.txt").data, QByteArray("Hello VFS\n"));
    QCOMPARE(readFile(path), data);
}

void VfsTest::emptyAndBom()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("empty.csv"));
    QVERIFY(writeFile(path, {}));
    Vfs vfs;
    QVERIFY(vfs.load(path).isEmpty());
    QCOMPARE(vfs.nodes().size(), 1);
    QVERIFY(vfs.nodes().value("/").directory);
    QVERIFY(writeFile(path, QByteArray::fromHex("efbbbf") + "path,type,data\r\n/x,file,\r\n"));
    QVERIFY(vfs.load(path).isEmpty());
    QCOMPARE(vfs.nodes().size(), 2);
    QVERIFY(vfs.nodes().value("/x").data.isEmpty());
}

void VfsTest::parentOrder()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("order.csv"));
    QVERIFY(writeFile(path, "path,type,data\n/a/b,file,\n/a,dir,\n"));
    Vfs vfs;
    QVERIFY(vfs.load(path).isEmpty());
    QCOMPARE(vfs.nodes().size(), 3);
}

void VfsTest::missingFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    Vfs vfs;
    QVERIFY(!vfs.load(directory.filePath("missing.csv")).isEmpty());
    QCOMPARE(vfs.nodes().size(), 1);
    QVERIFY(!vfs.load(directory.path()).isEmpty());
}

void VfsTest::resetAndReload()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("copy.csv"));
    QVERIFY(writeFile(path, readFile(QStringLiteral("vfs/nested.csv"))));
    Vfs vfs;
    QVERIFY(vfs.load(path).isEmpty());
    QCOMPARE(vfs.nodes().size(), 6);
    QVERIFY(vfs.reset().isEmpty());
    QCOMPARE(vfs.nodes().size(), 1);
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.size(), 0);
    file.close();
    Vfs reloaded;
    QVERIFY(reloaded.load(path).isEmpty());
    QCOMPARE(reloaded.nodes().size(), 1);
}

void VfsTest::resetFailure()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("copy.csv"));
    QVERIFY(writeFile(path, readFile(QStringLiteral("vfs/files.csv"))));
    Vfs vfs;
    QVERIFY(vfs.load(path).isEmpty());
    QVERIFY(QFile::remove(path));
    QVERIFY(QDir().mkdir(path));
    QVERIFY(!vfs.reset().isEmpty());
    QCOMPARE(vfs.nodes().size(), 5);
}

void VfsTest::initCommand()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("copy.csv"));
    const QByteArray original = readFile(QStringLiteral("vfs/files.csv"));
    QVERIFY(writeFile(path, original));
    Shell shell;
    QVERIFY(shell.loadVfs(path).isEmpty());
    QVERIFY(shell.execute(QStringLiteral("vfs-init extra")).error);
    QCOMPARE(readFile(path), original);
    QCOMPARE(shell.vfs().nodes().size(), 5);
    const auto result = shell.execute(QStringLiteral("vfs-init"));
    QVERIFY(!result.error);
    QVERIFY(result.output.contains(QStringLiteral("Каталогов: 1, файлов: 0")));
    QCOMPARE(shell.vfs().nodes().size(), 1);
    QCOMPARE(QFile(path).size(), 0);
    Shell unconfigured;
    QVERIFY(unconfigured.execute(QStringLiteral("vfs-init")).error);
}

QTEST_GUILESS_MAIN(VfsTest)
#include "test_vfs.moc"
