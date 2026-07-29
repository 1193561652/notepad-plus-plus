#include "UserDefinedLexer.h"

#include <QFont>
#include <cstdio>

namespace {
enum UdlStyle {
    DefaultStyle = 0,
    CommentStyle = 1,
    LineCommentStyle = 2,
    NumberStyle = 3,
    Keyword1Style = 4,
    Keyword8Style = 11,
    OperatorStyle = 12,
    FolderCode1Style = 13,
    FolderCode2Style = 14,
    FolderCommentStyle = 15,
    Delimiter1Style = 16,
    Delimiter8Style = 23,
    IdentifierStyle = 24
};

QByteArray lexerKeywordList(const QByteArray& source)
{
    QByteArray result;
    bool doubleQuoted = false;
    bool singleQuoted = false;
    bool nonWhitespaceFound = false;
    for (int i = 0; i < source.size(); ++i) {
        const char ch = source.at(i);
        if (!singleQuoted && ch == '"') {
            doubleQuoted = !doubleQuoted;
            continue;
        }
        if (!doubleQuoted && ch == '\'') {
            singleQuoted = !singleQuoted;
            continue;
        }
        if (ch == '\\' && i + 1 < source.size()
            && (source.at(i + 1) == '"' || source.at(i + 1) == '\''
                || source.at(i + 1) == '\\')) {
            result.append(source.at(++i));
            continue;
        }
        if (doubleQuoted || singleQuoted) {
            if (ch > ' ') {
                result.append(ch);
                nonWhitespaceFound = true;
            } else if (nonWhitespaceFound && i + 1 < source.size()
                       && source.at(i + 1) > ' ') {
                result.append(doubleQuoted ? '\v' : '\b');
            }
        } else {
            result.append(ch);
        }
    }
    return result;
}
}

UserDefinedLexer::UserDefinedLexer(
    const UserLangDesc& language, QObject* parent)
    : QsciLexer(parent), _language(language),
      _languageName(language.name.toUtf8())
{
    for (int i = 0; i < 28; ++i)
        _keywordLists[i] = language.keywordLists[i].toUtf8();
    const int lexerLists[15] = {
        9, 13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24, 25, 26
    };
    for (int index : lexerLists)
        _keywordLists[index] = lexerKeywordList(_keywordLists[index]);
    applyConfiguredStyles();
}

const char* UserDefinedLexer::language() const
{
    return _languageName.constData();
}

const char* UserDefinedLexer::lexer() const
{
    return "user";
}

const char* UserDefinedLexer::keywords(int set) const
{
    static const int listMap[15] = {
        9, 13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24, 25, 26
    };
    if (set < 1 || set > 15)
        return nullptr;
    const QByteArray& keywords = _keywordLists[listMap[set - 1]];
    return keywords.isEmpty() ? nullptr : keywords.constData();
}

bool UserDefinedLexer::caseSensitive() const
{
    return _language.caseSensitive;
}

QString UserDefinedLexer::description(int style) const
{
    if (style == DefaultStyle) return tr("Default");
    if (style == CommentStyle) return tr("Block comment");
    if (style == LineCommentStyle) return tr("Line comment");
    if (style == NumberStyle) return tr("Number");
    if (style >= Keyword1Style && style <= Keyword8Style)
        return tr("Keyword %1").arg(style - Keyword1Style + 1);
    if (style == OperatorStyle) return tr("Operator");
    if (style == FolderCode1Style) return tr("Folder in code 1");
    if (style == FolderCode2Style) return tr("Folder in code 2");
    if (style == FolderCommentStyle) return tr("Folder in comment");
    if (style >= Delimiter1Style && style <= Delimiter8Style)
        return tr("Delimiter %1").arg(style - Delimiter1Style + 1);
    if (style == IdentifierStyle) return tr("Identifier");
    return QString();
}

void UserDefinedLexer::refreshProperties()
{
    const QByteArray ignored = _language.caseSensitive ? "0" : "1";
    const QByteArray foldCompact = _language.foldCompact ? "1" : "0";
    const QByteArray foldComments = _language.foldComments ? "1" : "0";
    emit propertyChanged("fold", "1");
    emit propertyChanged("userDefine.isCaseIgnored", ignored.constData());
    emit propertyChanged("userDefine.foldCompact", foldCompact.constData());
    emit propertyChanged(
        "userDefine.allowFoldOfComments", foldComments.constData());
    const QByteArray lexerId = QByteArray::number(
        reinterpret_cast<quintptr>(this));
    emit propertyChanged("userDefine.udlName", lexerId.constData());
    emit propertyChanged("userDefine.currentBufferID", lexerId.constData());
    for (int i = 0; i < 8; ++i) {
        const QByteArray property =
            QByteArray("userDefine.prefixKeywords") + QByteArray::number(i + 1);
        emit propertyChanged(
            property.constData(), _language.prefixKeywords[i] ? "1" : "0");
    }
    const struct {
        const char* property;
        int list;
    } mappedProperties[] = {
        {"userDefine.comments", 0},
        {"userDefine.numberPrefix1", 1},
        {"userDefine.numberPrefix2", 2},
        {"userDefine.numberExtras1", 3},
        {"userDefine.numberExtras2", 4},
        {"userDefine.numberSuffix1", 5},
        {"userDefine.numberSuffix2", 6},
        {"userDefine.numberRange", 7},
        {"userDefine.operators1", 8},
        {"userDefine.foldersInCode1Open", 10},
        {"userDefine.foldersInCode1Middle", 11},
        {"userDefine.foldersInCode1Close", 12},
        {"userDefine.delimiters", 27}
    };
    for (const auto& mapped : mappedProperties)
        emit propertyChanged(
            mapped.property, _keywordLists[mapped.list].constData());
    for (const WordsStyle& style : _language.styles) {
        char property[32];
        std::snprintf(
            property, sizeof(property), "userDefine.nesting.%02d",
            style.styleID);
        const QByteArray value = QByteArray::number(style.nesting);
        emit propertyChanged(property, value.constData());
    }
}

void UserDefinedLexer::applyConfiguredStyles()
{
    for (const WordsStyle& style : _language.styles) {
        if (style.hasFg) setColor(style.fgColor, style.styleID);
        if (style.hasBg) setPaper(style.bgColor, style.styleID);
        QFont font = defaultFont(style.styleID);
        if (!style.fontName.isEmpty()) font.setFamily(style.fontName);
        if (style.fontSize > 0) font.setPointSize(style.fontSize);
        font.setBold(style.fontStyle & 1);
        font.setItalic(style.fontStyle & 2);
        font.setUnderline(style.fontStyle & 4);
        setFont(font, style.styleID);
    }
}
