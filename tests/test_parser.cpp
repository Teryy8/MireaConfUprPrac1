#include "commandparser.h"

#include <QtTest>

class ParserTest : public QObject {
    Q_OBJECT

private slots:
    void parse_data();
    void parse();
    void syntaxError_data();
    void syntaxError();
    void realEnvironment();
};

void ParserTest::parse_data()
{
    QTest::addColumn<QString>("line");
    QTest::addColumn<QStringList>("expected");
    QTest::newRow("empty") << QString() << QStringList{};
    QTest::newRow("whitespace") << QStringLiteral("  \t ") << QStringList{};
    QTest::newRow("simple") << QStringLiteral(" ls  -al \t /tmp ")
                           << QStringList{"ls", "-al", "/tmp"};
    QTest::newRow("variable") << QStringLiteral("cd $HOME") << QStringList{"cd", "/home/test"};
    QTest::newRow("braces") << QStringLiteral("cd ${HOME}/docs") << QStringList{"cd", "/home/test/docs"};
    QTest::newRow("single-quotes") << QStringLiteral("cd '$HOME'") << QStringList{"cd", "$HOME"};
    QTest::newRow("double-quotes") << QStringLiteral("cd \"$SPACE\"") << QStringList{"cd", "a b"};
    QTest::newRow("split-unquoted") << QStringLiteral("ls $SPACE") << QStringList{"ls", "a", "b"};
    QTest::newRow("split-attached") << QStringLiteral("ls pre${SPACE}post")
                                   << QStringList{"ls", "prea", "bpost"};
    QTest::newRow("quoted-path") << QStringLiteral("cd \"my folder\"") << QStringList{"cd", "my folder"};
    QTest::newRow("escaped-space") << QStringLiteral("cd my\\ folder") << QStringList{"cd", "my folder"};
    QTest::newRow("escaped-dollar") << QStringLiteral("ls \\$HOME") << QStringList{"ls", "$HOME"};
    QTest::newRow("double-escape") << QStringLiteral("ls \"\\$HOME\"") << QStringList{"ls", "$HOME"};
    QTest::newRow("literal-backslash") << QStringLiteral("ls \"a\\b\"") << QStringList{"ls", "a\\b"};
    QTest::newRow("empty-argument") << QStringLiteral("ls '' \"\"") << QStringList{"ls", "", ""};
    QTest::newRow("unknown-unquoted") << QStringLiteral("cd $MISSING") << QStringList{"cd"};
    QTest::newRow("unknown-quoted") << QStringLiteral("cd \"$MISSING\"") << QStringList{"cd", ""};
    QTest::newRow("no-recursive-expansion") << QStringLiteral("ls $LITERAL") << QStringList{"ls", "$HOME"};
    QTest::newRow("join-quotes") << QStringLiteral("cd pre' folder'post")
                                << QStringList{"cd", "pre folderpost"};
    QTest::newRow("literal-dollar") << QStringLiteral("ls $ $1") << QStringList{"ls", "$", "$1"};
    QTest::newRow("unicode") << QStringLiteral("cd \"Мои файлы\"") << QStringList{"cd", "Мои файлы"};
    QTest::newRow("variable-command") << QStringLiteral("$CMD") << QStringList{"ls"};
}

void ParserTest::parse()
{
    QFETCH(QString, line);
    QFETCH(QStringList, expected);
    QProcessEnvironment environment;
    environment.insert("HOME", "/home/test");
    environment.insert("SPACE", "a b");
    environment.insert("LITERAL", "$HOME");
    environment.insert("CMD", "ls");
    const ParseResult result = CommandParser(environment).parse(line);
    QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
    QCOMPARE(result.words, expected);
}

void ParserTest::syntaxError_data()
{
    QTest::addColumn<QString>("line");
    QTest::newRow("single-quote") << QStringLiteral("cd 'folder");
    QTest::newRow("double-quote") << QStringLiteral("cd \"folder");
    QTest::newRow("escape") << QStringLiteral("ls test\\");
    QTest::newRow("brace") << QStringLiteral("ls ${HOME");
    QTest::newRow("empty-name") << QStringLiteral("ls ${}");
    QTest::newRow("invalid-name") << QStringLiteral("ls ${1HOME}");
    QTest::newRow("invalid-character") << QStringLiteral("ls ${HOME-PATH}");
}

void ParserTest::syntaxError()
{
    QFETCH(QString, line);
    const ParseResult result = CommandParser(QProcessEnvironment{}).parse(line);
    QVERIFY(!result.error.isEmpty());
    QVERIFY(result.words.isEmpty());
}

void ParserTest::realEnvironment()
{
    const QByteArray name("SHELL_EMULATOR_STAGE1_TEST");
    const bool existed = qEnvironmentVariableIsSet(name.constData());
    const QByteArray previous = qgetenv(name.constData());
    qputenv(name.constData(), "real process value");
    const ParseResult result = CommandParser().parse(QStringLiteral("cd \"$SHELL_EMULATOR_STAGE1_TEST\""));
    if (existed) {
        qputenv(name.constData(), previous);
    } else {
        qunsetenv(name.constData());
    }
    QCOMPARE(result.words, (QStringList{"cd", "real process value"}));
}

QTEST_GUILESS_MAIN(ParserTest)
#include "test_parser.moc"
