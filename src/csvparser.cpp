#include "csvparser.h"

CsvRow parseCsvRow(const QString &line)
{
    enum class State { Start, Unquoted, Quoted, AfterQuote };
    State state = State::Start;
    CsvRow result;
    QString field;
    for (const QChar ch : line) {
        if (state == State::Quoted) {
            if (ch == u'"') {
                state = State::AfterQuote;
            } else {
                field += ch;
            }
        } else if (state == State::AfterQuote && ch == u'"') {
            field += ch;
            state = State::Quoted;
        } else if (ch == u',') {
            result.fields.append(field);
            field.clear();
            state = State::Start;
        } else if (state == State::Start && ch == u'"') {
            state = State::Quoted;
        } else if (state == State::AfterQuote || ch == u'"') {
            return {{}, QStringLiteral("неверная запись кавычек в CSV")};
        } else {
            field += ch;
            state = State::Unquoted;
        }
    }
    if (state == State::Quoted) {
        return {{}, QStringLiteral("незакрытая кавычка в CSV")};
    }
    result.fields.append(field);
    return result;
}
