#include "UserDefinedLexer.h"

#include "ScintillaEditView.h"

#include <QFont>
#include <cstdio>

namespace {
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

void setProperty(ScintillaEditView& view, const QByteArray& key,
                 const QByteArray& value)
{
    view.execute(SCI_SETPROPERTY,
        reinterpret_cast<uptr_t>(key.constData()),
        reinterpret_cast<sptr_t>(value.constData()));
}

int sciColour(const QColor& color)
{
    return color.red() | (color.green() << 8) | (color.blue() << 16);
}
}

void UserDefinedLexer::configure(ScintillaEditView& view,
                                 const UserLangDesc& language)
{
    QByteArray lists[28];
    for (int i = 0; i < 28; ++i)
        lists[i] = language.keywordLists[i].toUtf8();
    const int lexerLists[15] = {
        9, 13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24, 25, 26
    };
    for (int i = 0; i < 15; ++i) {
        lists[lexerLists[i]] = lexerKeywordList(lists[lexerLists[i]]);
        view.execute(SCI_SETKEYWORDS, i,
            reinterpret_cast<sptr_t>(lists[lexerLists[i]].constData()));
    }

    setProperty(view, "fold", "1");
    setProperty(view, "userDefine.isCaseIgnored",
                language.caseSensitive ? "0" : "1");
    setProperty(view, "userDefine.foldCompact",
                language.foldCompact ? "1" : "0");
    setProperty(view, "userDefine.allowFoldOfComments",
                language.foldComments ? "1" : "0");
    for (int i = 0; i < 8; ++i) {
        setProperty(view, QByteArray("userDefine.prefixKeywords")
                    + QByteArray::number(i + 1),
                    language.prefixKeywords[i] ? "1" : "0");
    }
    const struct { const char* property; int list; } properties[] = {
        {"userDefine.comments", 0}, {"userDefine.numberPrefix1", 1},
        {"userDefine.numberPrefix2", 2}, {"userDefine.numberExtras1", 3},
        {"userDefine.numberExtras2", 4}, {"userDefine.numberSuffix1", 5},
        {"userDefine.numberSuffix2", 6}, {"userDefine.numberRange", 7},
        {"userDefine.operators1", 8}, {"userDefine.foldersInCode1Open", 10},
        {"userDefine.foldersInCode1Middle", 11},
        {"userDefine.foldersInCode1Close", 12},
        {"userDefine.delimiters", 27}
    };
    for (const auto& property : properties)
        setProperty(view, property.property, lists[property.list]);

    for (const WordsStyle& style : language.styles) {
        if (style.hasFg)
            view.execute(SCI_STYLESETFORE, style.styleID, sciColour(style.fgColor));
        if (style.hasBg)
            view.execute(SCI_STYLESETBACK, style.styleID, sciColour(style.bgColor));
        if (!style.fontName.isEmpty()) {
            const QByteArray family = style.fontName.toUtf8();
            view.execute(SCI_STYLESETFONT, style.styleID,
                reinterpret_cast<sptr_t>(family.constData()));
        }
        if (style.fontSize > 0)
            view.execute(SCI_STYLESETSIZE, style.styleID, style.fontSize);
        view.execute(SCI_STYLESETBOLD, style.styleID, style.fontStyle & 1);
        view.execute(SCI_STYLESETITALIC, style.styleID, style.fontStyle & 2);
        view.execute(SCI_STYLESETUNDERLINE, style.styleID, style.fontStyle & 4);

        char property[32];
        std::snprintf(property, sizeof(property), "userDefine.nesting.%02d",
                      style.styleID);
        setProperty(view, property, QByteArray::number(style.nesting));
    }
}
