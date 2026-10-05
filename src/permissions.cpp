#include "permissions.h"

#include <QStringList>

namespace {
unsigned int userMask(QChar ch)
{
    if (ch == u'u') return 0700;
    if (ch == u'g') return 0070;
    if (ch == u'o') return 0007;
    if (ch == u'a') return 0777;
    return 0;
}

unsigned int specialMask(unsigned int users)
{
    return ((users & 0700) ? 04000 : 0) | ((users & 0070) ? 02000 : 0) | ((users & 0007) ? 01000 : 0);
}
} // namespace

bool PermissionMode::parse(const QString &text)
{
    numeric = -1;
    changes.clear();
    if (text.isEmpty()) return false;
    if (text.front().isDigit()) {
        if (text.size() > 4) return false;
        unsigned int value = 0;
        for (QChar ch : text) {
            if (ch < u'0' || ch > u'7') return false;
            value = value * 8 + ch.unicode() - u'0';
        }
        numeric = static_cast<int>(value);
        return true;
    }
    for (const QString &clause : text.split(u',')) {
        qsizetype position = 0;
        unsigned int users = 0;
        while (position < clause.size() && userMask(clause[position])) {
            users |= userMask(clause[position++]);
        }
        if (!users) users = 0777;
        bool hasOperation = false;
        while (position < clause.size()) {
            const QChar operation = clause[position++];
            if (operation != u'+' && operation != u'-' && operation != u'=') return false;
            QString permissions;
            while (position < clause.size() && clause[position] != u'+'
                   && clause[position] != u'-' && clause[position] != u'=') {
                const QChar ch = clause[position++];
                if (!QStringLiteral("rwxXstugo").contains(ch)) return false;
                permissions += ch;
            }
            if ((permissions.contains(u'u') || permissions.contains(u'g') || permissions.contains(u'o'))
                && permissions.size() != 1) return false;
            changes.append({users, operation, permissions});
            hasOperation = true;
        }
        if (!hasOperation) return false;
    }
    return true;
}

unsigned int PermissionMode::apply(unsigned int current, bool directory) const
{
    if (numeric >= 0) return static_cast<unsigned int>(numeric);
    // X проверяет тип и исходные права до выполнения символьных операций.
    const bool executable = directory || (current & 0111);
    for (const PermissionChange &change : changes) {
        unsigned int bits = 0;
        for (QChar ch : change.permissions) {
            if (ch == u'r') bits |= 0444;
            if (ch == u'w') bits |= 0222;
            if (ch == u'x' || (ch == u'X' && executable)) bits |= 0111;
            if (ch == u'u' || ch == u'g' || ch == u'o') {
                const int shift = ch == u'u' ? 6 : (ch == u'g' ? 3 : 0);
                const unsigned int copied = (current >> shift) & 7;
                bits |= (copied << 6) | (copied << 3) | copied;
            }
        }
        bits &= change.users;
        if (change.permissions.contains(u's')) {
            bits |= specialMask(change.users) & 06000;
        }
        if (change.permissions.contains(u't') && (change.users & 0007)) bits |= 01000;
        if (change.operation == u'=') {
            current = (current & ~(change.users | specialMask(change.users))) | bits;
        } else if (change.operation == u'+') {
            current |= bits;
        } else {
            current &= ~bits;
        }
    }
    return current;
}
