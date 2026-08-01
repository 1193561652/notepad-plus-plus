#include <QApplication>
#include <QByteArray>
#include <ScintillaEditBase.h>

#include "../src/ScintillaComponent/ScintillaTextSearch.h"

#include <iostream>

namespace {

bool expect(bool condition, const char* message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}

long search(ScintillaEditBase& editor, const QByteArray& pattern,
            long start, long end, int extraFlags = 0)
{
    editor.send(SCI_SETSEARCHFLAGS, SCFIND_REGEXP | extraFlags);
    editor.send(SCI_SETTARGETSTART, start);
    editor.send(SCI_SETTARGETEND, end);
    return editor.send(SCI_SEARCHINTARGET, pattern.size(),
                       reinterpret_cast<sptr_t>(pattern.constData()));
}

} // namespace

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    ScintillaEditBase editor;
    const QByteArray initial("foo bar\r\nfoobar\nalpha\nbeta");
    editor.send(SCI_SETCODEPAGE, SC_CP_UTF8);
    editor.send(SCI_SETTEXT, 0,
                reinterpret_cast<sptr_t>(initial.constData()));

    const long documentLength =
        editor.send(SCI_GETLENGTH);

    bool ok = true;
    const long lookbehind = search(editor, QByteArray("(?<=foo)bar"),
                                   0, documentLength);
    ok &= expect(lookbehind == 12,
                 "Boost lookbehind should find 'bar' in 'foobar'");

    const long newline = search(editor, QByteArray("\\R"),
                                0, documentLength);
    ok &= expect(newline == 7, "Boost \\R should match CRLF");
    ok &= expect(editor.send(
                     SCI_GETTARGETEND) == 9,
                 "Boost \\R should consume CRLF as one match");

    const long capture = search(editor, QByteArray("(alpha)\\R(beta)"),
                                0, documentLength);
    ok &= expect(capture >= 0, "capturing expression should match");
    if (capture >= 0) {
        const QByteArray replacement("$2/$1");
        editor.send(SCI_REPLACETARGETRE, replacement.size(),
                    reinterpret_cast<sptr_t>(replacement.constData()));
        const sptr_t length = editor.send(SCI_GETLENGTH);
        QByteArray result(static_cast<int>(length) + 1, '\0');
        editor.send(SCI_GETTEXT, length + 1,
                    reinterpret_cast<sptr_t>(result.data()));
        ok &= expect(QString::fromUtf8(result).endsWith(QStringLiteral("beta/alpha")),
                     "Boost replacement should expand capture groups");
    }

    QString plainText = QStringLiteral("foo\r\nbar foo\nbar");
    const QList<ScintillaTextMatch> matches =
        ScintillaTextSearch::findAll(
            plainText, QStringLiteral("(?<=foo)\\Rbar"),
            true, true, false, false);
    ok &= expect(matches.size() == 2,
                 "plain-text routes should use Boost.Regex semantics");
    const int replaced = ScintillaTextSearch::replaceAll(
        plainText, QStringLiteral("(foo)\\R(bar)"),
        QStringLiteral("$2/$1"), true, true, false, false);
    ok &= expect(replaced == 2
                 && plainText == QStringLiteral("bar/foo bar/foo"),
                 "plain-text replacement should use Boost capture syntax");

    const QList<ScintillaTextMatch> emptyMatches =
        ScintillaTextSearch::findAll(
            QStringLiteral("ab"), QStringLiteral("(?=.)"),
            true, true, false, false);
    ok &= expect(emptyMatches.size() == 2,
                 "Boost empty matches should advance by character");
    const QList<ScintillaTextMatch> dotAllMatches =
        ScintillaTextSearch::findAll(
            QStringLiteral("a\nb"), QStringLiteral("a.b"),
            true, true, false, true);
    ok &= expect(dotAllMatches.size() == 1,
                 "dot-matches-newline should use the Scintilla extension flag");
    const QList<ScintillaTextMatch> utf8ColumnMatches =
        ScintillaTextSearch::findAll(
            QString::fromUtf8("\xC3\xA9 foo"), QStringLiteral("foo"),
            false, true, false, false);
    ok &= expect(
        utf8ColumnMatches.size() == 1
            && utf8ColumnMatches.first().byteColumn == 3,
        "Finder match columns should preserve UTF-8 byte offsets");

    if (ok)
        std::cout << "Boost.Regex Scintilla integration passed\n";
    return ok ? 0 : 1;
}
