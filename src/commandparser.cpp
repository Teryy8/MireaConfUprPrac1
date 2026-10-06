/*
 * Посимвольно разбирает команду с учётом пробелов, кавычек и экранирования.
 * Раскрывает переменные окружения и сообщает о синтаксических ошибках.
 */

#include "commandparser.h"

#include <utility>

namespace {
enum class Quote { None, Single, Double };

bool isNameStart(QChar ch)
{
    return ch == u'_' || (ch >= u'A' && ch <= u'Z') || (ch >= u'a' && ch <= u'z');
}

bool isNameChar(QChar ch)
{
    return isNameStart(ch) || (ch >= u'0' && ch <= u'9');
}

QString variableName(const QString &line, qsizetype &position, QString &error)
{
    const qsizetype start = position + 1;
    if (start >= line.size()) {
        return {};
    }
    if (line[start] == u'{') {
        const qsizetype end = line.indexOf(u'}', start + 1);
        if (end < 0) {
            error = QStringLiteral("Ошибка синтаксиса: переменная ${...} не закрыта.");
            return {};
        }
        const QString name = line.mid(start + 1, end - start - 1);
        if (name.isEmpty() || !isNameStart(name.front())) {
            error = QStringLiteral("Ошибка синтаксиса: неверное имя переменной.");
            return {};
        }
        for (const QChar ch : name) {
            if (!isNameChar(ch)) {
                error = QStringLiteral("Ошибка синтаксиса: неверное имя переменной.");
                return {};
            }
        }
        position = end;
        return name;
    }
    if (!isNameStart(line[start])) {
        return {};
    }
    qsizetype end = start + 1;
    while (end < line.size() && isNameChar(line[end])) {
        ++end;
    }
    position = end - 1;
    return line.mid(start, end - start);
}
} // namespace

CommandParser::CommandParser(QProcessEnvironment environment)
    : environment_(std::move(environment))
{
}

ParseResult CommandParser::parse(const QString &line) const
{
    ParseResult result;
    QString word;
    bool started = false;
    Quote quote = Quote::None;
    const auto flush = [&] {
        if (started) {
            result.words.append(word);
            word.clear();
            started = false;
        }
    };

    for (qsizetype i = 0; i < line.size(); ++i) {
        const QChar ch = line[i];
        if (quote == Quote::Single) {
            if (ch == u'\'') {
                quote = Quote::None;
            } else {
                word += ch;
            }
        } else if (ch == u'\\') {
            if (i + 1 >= line.size()) {
                return {{}, QStringLiteral("Ошибка синтаксиса: незавершённое экранирование.")};
            }
            const QChar next = line[i + 1];
            if (quote == Quote::Double && next != u'$' && next != u'"' && next != u'\\') {
                word += ch;
            } else {
                word += next;
                ++i;
            }
            started = true;
        } else if (ch == u'"') {
            quote = quote == Quote::Double ? Quote::None : Quote::Double;
            started = true;
        } else if (quote == Quote::None && ch == u'\'') {
            quote = Quote::Single;
            started = true;
        } else if (ch == u'$') {
            const QString name = variableName(line, i, result.error);
            if (!result.error.isEmpty()) {
                return {{}, result.error};
            }
            if (name.isEmpty()) {
                word += ch;
                started = true;
                continue;
            }
            const QString value = environment_.value(name);
            for (const QChar expanded : value) {
                if (quote == Quote::None && (expanded == u' ' || expanded == u'\t' || expanded == u'\n')) {
                    flush();
                } else {
                    word += expanded;
                    started = true;
                }
            }
        } else if (quote == Quote::None && ch.isSpace()) {
            flush();
        } else {
            word += ch;
            started = true;
        }
    }
    if (quote != Quote::None) {
        return {{}, QStringLiteral("Ошибка синтаксиса: незакрытая кавычка.")};
    }
    flush();
    return result;
}
