#include "ScintillaTextSearch.h"
#include <BoostRegexSearch.h>

#include <QByteArray>
#include <ScintillaEditBase.h>

namespace {

int searchFlags(bool regex, bool matchCase, bool wholeWord,
                bool dotMatchesNewline)
{
    unsigned int flags = 0;
    if (matchCase)
        flags |= SCFIND_MATCHCASE;
    if (wholeWord)
        flags |= SCFIND_WHOLEWORD;
    if (regex) {
        flags |= SCFIND_REGEXP;
        flags |= SCFIND_REGEXP_EMPTYMATCH_ALL;
        flags |= SCFIND_REGEXP_SKIPCRLFASONE;
        if (dotMatchesNewline)
            flags |= SCFIND_REGEXP_DOTMATCHESNL;
    }
    return static_cast<int>(flags);
}

void setEditorText(ScintillaEditBase& editor, const QString& text)
{
    const QByteArray utf8 = text.toUtf8();
    editor.send(SCI_SETCODEPAGE, SC_CP_UTF8);
    editor.send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(utf8.constData()));
}

QString editorLineText(ScintillaEditBase& editor, int line)
{
    const sptr_t length = editor.send(SCI_LINELENGTH, line);
    QByteArray bytes(static_cast<int>(length) + 1, '\0');
    editor.send(SCI_GETLINE, line, reinterpret_cast<sptr_t>(bytes.data()));
    bytes.truncate(static_cast<int>(length));
    return QString::fromUtf8(bytes);
}

void prepare(ScintillaEditBase& editor, const QString& text, int flags)
{
    setEditorText(editor, text);
    editor.send(SCI_SETSEARCHFLAGS, flags);
}

long findTarget(ScintillaEditBase& editor, const QByteArray& pattern,
                long start, long end)
{
    editor.send(SCI_SETTARGETSTART, start);
    editor.send(SCI_SETTARGETEND, end);
    return editor.send(SCI_SEARCHINTARGET, pattern.size(),
        reinterpret_cast<sptr_t>(pattern.constData()));
}

long nextPosition(ScintillaEditBase& editor, long matchStart, long matchEnd,
                  long documentLength)
{
    if (matchEnd > matchStart)
        return matchEnd;
    if (matchStart >= documentLength)
        return documentLength + 1;
    return editor.send(SCI_POSITIONAFTER,
                                matchStart);
}

} // namespace

QList<ScintillaTextMatch> ScintillaTextSearch::findAll(
    const QString& text, const QString& searchText,
    bool regex, bool matchCase, bool wholeWord,
    bool dotMatchesNewline, bool* valid)
{
    QList<ScintillaTextMatch> matches;
    if (valid)
        *valid = true;
    if (searchText.isEmpty())
        return matches;

    ScintillaEditBase editor;
    prepare(editor, text, searchFlags(regex, matchCase, wholeWord,
                                      dotMatchesNewline));
    const QByteArray pattern = searchText.toUtf8();
    const long documentLength =
        editor.send(SCI_GETLENGTH);
    long position = 0;
    while (position <= documentLength) {
        const long found = findTarget(editor, pattern, position,
                                      documentLength);
        if (found < 0) {
            if (found < -1 && valid)
                *valid = false;
            break;
        }
        const long end =
            editor.send(SCI_GETTARGETEND);
        const int line = static_cast<int>(editor.send(
            SCI_LINEFROMPOSITION, found));

        ScintillaTextMatch match;
        match.byteStart = static_cast<int>(found);
        match.byteLength = static_cast<int>(end - found);
        match.line = line;
        const long lineStart = editor.send(
            SCI_POSITIONFROMLINE, line);
        match.byteColumn = static_cast<int>(found - lineStart);
        match.lineText = editorLineText(editor, line);
        while (!match.lineText.isEmpty()
               && (match.lineText.endsWith('\n')
                   || match.lineText.endsWith('\r')))
            match.lineText.chop(1);
        matches.append(match);
        position = nextPosition(editor, found, end, documentLength);
    }
    return matches;
}

int ScintillaTextSearch::replaceAll(
    QString& text, const QString& searchText, const QString& replacement,
    bool regex, bool matchCase, bool wholeWord, bool dotMatchesNewline)
{
    if (searchText.isEmpty())
        return 0;

    ScintillaEditBase editor;
    prepare(editor, text, searchFlags(regex, matchCase, wholeWord,
                                      dotMatchesNewline));
    const QByteArray pattern = searchText.toUtf8();
    const QByteArray replacementBytes = replacement.toUtf8();
    long documentLength =
        editor.send(SCI_GETLENGTH);
    long position = 0;
    int count = 0;
    while (position <= documentLength) {
        const long found = findTarget(editor, pattern, position,
                                      documentLength);
        if (found < -1)
            return -1;
        if (found < 0)
            break;

        const long matchEnd =
            editor.send(SCI_GETTARGETEND);
        const bool emptyMatch = matchEnd == found;
        const long replacementLength = editor.send(
            regex ? SCI_REPLACETARGETRE
                  : SCI_REPLACETARGET,
            replacementBytes.size(),
            reinterpret_cast<sptr_t>(replacementBytes.constData()));
        documentLength += replacementLength - (matchEnd - found);
        ++count;
        const long replacementEnd = found + replacementLength;
        position = emptyMatch
            ? nextPosition(editor, replacementEnd, replacementEnd,
                           documentLength)
            : replacementEnd;
    }
    const sptr_t finalLength = editor.send(SCI_GETLENGTH);
    QByteArray finalText(static_cast<int>(finalLength) + 1, '\0');
    editor.send(SCI_GETTEXT, finalLength + 1,
                reinterpret_cast<sptr_t>(finalText.data()));
    finalText.truncate(static_cast<int>(finalLength));
    text = QString::fromUtf8(finalText);
    return count;
}

bool ScintillaTextSearch::replaceWholeMatch(
    QString& text, const QString& searchText, const QString& replacement,
    bool regex, bool matchCase, bool wholeWord, bool dotMatchesNewline)
{
    ScintillaEditBase editor;
    prepare(editor, text, searchFlags(regex, matchCase, wholeWord,
                                      dotMatchesNewline));
    const QByteArray pattern = searchText.toUtf8();
    const long documentLength =
        editor.send(SCI_GETLENGTH);
    const long found = findTarget(editor, pattern, 0, documentLength);
    const long end = found >= 0
        ? editor.send(SCI_GETTARGETEND)
        : -1;
    if (found != 0 || end != documentLength)
        return false;

    const QByteArray replacementBytes = replacement.toUtf8();
    editor.send(
        regex ? SCI_REPLACETARGETRE
              : SCI_REPLACETARGET,
        replacementBytes.size(),
        reinterpret_cast<sptr_t>(replacementBytes.constData()));
    const sptr_t finalLength = editor.send(SCI_GETLENGTH);
    QByteArray finalText(static_cast<int>(finalLength) + 1, '\0');
    editor.send(SCI_GETTEXT, finalLength + 1,
                reinterpret_cast<sptr_t>(finalText.data()));
    finalText.truncate(static_cast<int>(finalLength));
    text = QString::fromUtf8(finalText);
    return true;
}
