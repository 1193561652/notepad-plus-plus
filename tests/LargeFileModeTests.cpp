#include "ScintillaComponent/FileManager.h"
#include "MISC/TextFileCodec.h"
#include "ScintillaComponent/ScintillaEditView.h"
#include "Parameters.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextCodec>
#include <QMetaObject>
#include <QMouseEvent>
#include <Scintilla.h>
#include <SciLexer.h>
#include <Lexilla.h>
#include <cstdio>

namespace {

constexpr int IoBlockSize = 128 * 1024 + 4;

bool check(bool condition, const char* message)
{
    if (!condition)
        std::fprintf(stderr, "FAILED: %s\n", message);
    return condition;
}

bool writeBytes(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) &&
           file.write(bytes) == bytes.size();
}

QByteArray scintillaString(ScintillaEditView& view, unsigned int message,
                           uptr_t wParam)
{
    const sptr_t length = view.SendScintilla(message, wParam, 0);
    QByteArray value(static_cast<int>(length) + 1, '\0');
    view.SendScintilla(message, wParam,
                       reinterpret_cast<sptr_t>(value.data()));
    value.truncate(static_cast<int>(length));
    return value;
}

QByteArray lexerProperty(ScintillaEditView& view, const char* name)
{
    return scintillaString(view, SCI_GETPROPERTY,
        reinterpret_cast<uptr_t>(name));
}

bool verifyStreamingCase(
    const QString& path, const QByteArray& bytes,
    const QString& encoding, const QString& expected,
    bool editBeforeSave)
{
    if (!writeBytes(path, bytes))
        return check(false, "Could not create streaming test input");

    Buffer* buffer = MainFileManager.loadBuffer(path);
    if (!buffer)
        return check(false, "Could not register streaming test buffer");

    buffer->setLargeFile(true);
    buffer->setSourceFileSize(bytes.size());
    ScintillaEditView view;
    buffer->setView(&view);

    TextDecodingOptions options;
    options.filePath = path;
    QString error;
    bool ok = check(
        MainFileManager.loadBufferContent(
            buffer, &view, options, encoding, &error),
        qPrintable(QStringLiteral("Streaming load failed: %1").arg(error)));

    const qintptr documentOptions =
        view.SendScintillaNpp(SCI_GETDOCUMENTOPTIONS, 0, 0);
    ok &= check(
        (documentOptions & SC_DOCUMENTOPTION_STYLES_NONE) != 0 &&
        (documentOptions & SC_DOCUMENTOPTION_TEXT_LARGE) != 0,
        "Large Scintilla document options were not applied");
    ok &= check(view.isLargeFileMode() &&
                view.wrapMode() == WrapNone &&
                !view.hasLexer() &&
                view.autoCompletionSource() == ScintillaEditView::AcsNone,
                "Large-file feature degradation was not applied");
    const QString loadedText = view.text();
    if (loadedText != expected) {
        int mismatch = 0;
        while (mismatch < loadedText.size() &&
               mismatch < expected.size() &&
               loadedText.at(mismatch) == expected.at(mismatch))
            ++mismatch;
        std::fprintf(
            stderr,
            "DETAIL: %s load expected=%d actual=%d mismatch=%d "
            "expectedUcs=%04x actualUcs=%04x\n",
            qPrintable(encoding), expected.size(), loadedText.size(), mismatch,
            mismatch < expected.size() ? expected.at(mismatch).unicode() : 0,
            mismatch < loadedText.size() ? loadedText.at(mismatch).unicode() : 0);
    }
    ok &= check(loadedText == expected,
                "Streaming decode changed document text");

    ScintillaEditView clone;
    clone.setDocument(view.document());
    clone.setLargeFileMode(true);
    ok &= check(clone.text() == expected,
                "Cloned large document does not share content");

    QString savedExpected = expected;
    if (editBeforeSave) {
        view.setCurrentPositionNpp(0);
        ok &= check(view.findFirst(
                        QStringLiteral("tail"), false, true, true,
                        false, true),
                    "Large-document search failed");
        view.replace(QStringLiteral("finish"));
        savedExpected.replace(
            savedExpected.lastIndexOf(QStringLiteral("tail")), 4,
            QStringLiteral("finish"));
        view.insertText(7, QStringLiteral("X"));
        savedExpected.insert(7, QLatin1Char('X'));
    }

    const QString savedPath = path + QStringLiteral(".saved");
    error.clear();
    ok &= check(MainFileManager.saveBufferCopy(
                    buffer, savedPath, &error),
                qPrintable(QStringLiteral("Streaming save failed: %1")
                               .arg(error)));
    QFile saved(savedPath);
    ok &= check(saved.open(QIODevice::ReadOnly),
                "Could not read streaming save output");
    const DecodedTextFile decoded =
        TextFileCodec::decodeAs(saved.readAll(), encoding);
    ok &= check(decoded.isValid && decoded.text == savedExpected,
                "Streaming save changed document text");

    MainFileManager.closeBuffer(buffer);
    return ok;
}

} // namespace

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QTemporaryDir temporary;
    bool ok = check(temporary.isValid(),
                    "Could not create temporary test directory");
    NppParameters& parameters = NppParameters::getInstance();
    const QString settingsPath =
        temporary.filePath(QStringLiteral("settings"));
    ok &= check(QDir().mkpath(settingsPath),
                "Could not create temporary settings directory");
    ok &= check(parameters.setUserPathOverride(
                    settingsPath) &&
                parameters.load(),
                "Could not load runtime language configuration");

    {
        ScintillaEditView marginView;
        marginView.setText(QStringLiteral("first\nsecond\nthird"));
        marginView.setCursorPosition(0, 0);
        marginView.clearLexer();
        ok &= check(marginView.SendScintilla(
                        SCI_STYLEGETBACK, STYLE_LINENUMBER) == 0xE4E4E4,
                    "Clearing the lexer reset the configured line-number colour");
        ok &= check(marginView.SendScintilla(
                        SCI_GETMARGINBACKN, 1) == 0xE0E0E0,
                    "Bookmark margin did not use the configured background");
        ok &= check(marginView.viewport()->hasMouseTracking(),
                    "Scintilla viewport does not track margin hover");
        ok &= check(marginView.SendScintilla(
                        SCI_GETMARGINCURSORN, 1) == SC_CURSORREVERSEARROW,
                    "Bookmark margin does not use the original hover cursor");
        ok &= check(marginView.SendScintilla(
                        SCI_GETMARGINTYPEN, 1) == SC_MARGIN_COLOUR,
                    "Bookmark margin does not use its configured colour");
        ok &= check(marginView.SendScintilla(
                        SCI_GETMARGINMASKN, 1) == (1 << 1),
                    "Bookmark margin accepts unrelated markers");
        marginView.resize(480, 240);
        marginView.show();
        QApplication::processEvents();
        const int bookmarkX = marginView.marginWidth(0)
            + marginView.marginWidth(1) / 2;
        const int lineY = static_cast<int>(marginView.SendScintilla(
            SCI_POINTYFROMPOSITION, 0,
            marginView.positionFromLineIndex(1, 0))) + 4;
        QMouseEvent marginMove(QEvent::MouseMove,
                               QPointF(bookmarkX, lineY),
                               Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(marginView.viewport(), &marginMove);
        const QCursor marginCursor = marginView.viewport()->cursor();
        ok &= check(marginCursor.shape() == Qt::BitmapCursor
                        && !marginCursor.pixmap().isNull()
                        && marginCursor.hotSpot().x()
                            > marginCursor.pixmap().width() / 2,
                    "Bookmark margin does not use a right-facing pointer");
        QMouseEvent textMove(QEvent::MouseMove,
            QPointF(marginView.marginWidth(0) + marginView.marginWidth(1)
                    + marginView.marginWidth(2) + 20, lineY),
            Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(marginView.viewport(), &textMove);
        ok &= check(marginView.viewport()->cursor().shape() == Qt::IBeamCursor,
                    "Pointer did not return to text mode over the editor");
        const bool invoked = QMetaObject::invokeMethod(
            &marginView, "onMarginClicked", Qt::DirectConnection,
            Q_ARG(int, 1), Q_ARG(int, 2),
            Q_ARG(Qt::KeyboardModifiers, Qt::NoModifier));
        ok &= check(invoked, "Could not dispatch bookmark margin click");
        ok &= check((marginView.markersAtLine(2) & (1 << 1)) != 0,
                    "Margin click did not bookmark the clicked line");
        ok &= check((marginView.markersAtLine(0) & (1 << 1)) == 0,
                    "Margin click incorrectly bookmarked the caret line");
    }

    const QStringList builtinLanguages = {
        "php", "c", "cpp", "cs", "objc", "java", "rc", "html", "xml",
        "makefile", "pascal", "batch", "ini", "asp", "sql", "vb",
        "javascript", "css", "perl", "python", "lua", "tex", "fortran",
        "bash", "actionscript", "nsis", "tcl", "lisp", "scheme", "asm",
        "diff", "props", "postscript", "ruby", "smalltalk", "vhdl", "kix",
        "autoit", "caml", "ada", "verilog", "matlab", "haskell", "inno",
        "cmake", "yaml", "cobol", "gui4cli", "d", "powershell", "r", "jsp",
        "coffeescript", "json", "javascript.js", "fortran77", "baanc",
        "srec", "ihex", "tehex", "swift", "asn1", "avs", "blitzbasic",
        "purebasic", "freebasic", "csound", "erlang", "escript", "forth",
        "latex", "mmixal", "nim", "nncrontab", "oscript", "rebol",
        "registry", "rust", "spice", "txt2tags", "visualprolog",
        "typescript", "markdown", "searchResult"
    };
    for (const QString& language : builtinLanguages) {
        ok &= check(
            !ScintillaEditView::builtinLexerName(language).isEmpty(),
            qPrintable(QStringLiteral("Missing built-in lexer mapping: %1")
                           .arg(language)));
    }
    QString utf8Text(IoBlockSize - 1, QLatin1Char('a'));
    utf8Text += QChar(0x20AC);
    utf8Text += QStringLiteral("\r\ntail");
    ok &= verifyStreamingCase(
        temporary.filePath(QStringLiteral("split-utf8.txt")),
        utf8Text.toUtf8(), QStringLiteral("UTF-8"), utf8Text, true);

    QString utf16Text((IoBlockSize / 2) - 1, QLatin1Char('b'));
    const uint smilingFace = 0x1F642;
    utf16Text += QString::fromUcs4(&smilingFace, 1);
    utf16Text += QStringLiteral("\ntail");
    QByteArray utf16le;
    ok &= check(TextFileCodec::encode(
                    utf16Text, QStringLiteral("UTF-16LE"), true, &utf16le),
                "Could not encode UTF-16LE test input");
    ok &= verifyStreamingCase(
        temporary.filePath(QStringLiteral("split-utf16le.txt")),
        utf16le, QStringLiteral("UTF-16LE"), utf16Text, false);

    QByteArray utf16be;
    ok &= check(TextFileCodec::encode(
                    utf16Text, QStringLiteral("UTF-16BE"), true, &utf16be),
                "Could not encode UTF-16BE test input");
    ok &= verifyStreamingCase(
        temporary.filePath(QStringLiteral("split-utf16be.txt")),
        utf16be, QStringLiteral("UTF-16BE"), utf16Text, false);

    const QString legacyText =
        QString(IoBlockSize - 1, QLatin1Char('c')) + QChar(0x00E9) +
        QStringLiteral("\r\ntail");
    QTextCodec* legacyCodec = QTextCodec::codecForName("windows-1252");
    ok &= check(legacyCodec != nullptr, "windows-1252 codec is unavailable");
    if (legacyCodec) {
        ok &= verifyStreamingCase(
            temporary.filePath(QStringLiteral("split-legacy.txt")),
            legacyCodec->fromUnicode(legacyText),
            QStringLiteral("windows-1252"), legacyText, false);
    }

    const QString backupSource =
        temporary.filePath(QStringLiteral("backup-source.txt"));
    ok &= check(writeBytes(backupSource, QByteArray("previous")),
                "Could not create backup source");
    QString backupPath;
    QString backupError;
    ok &= check(FileManager::backupFileBeforeSave(
                    backupSource, 1, false, QString(),
                    QStringLiteral("2026-07-26_120000"),
                    &backupPath, &backupError),
                "Simple backup failed");
    QFile simpleBackup(backupPath);
    ok &= check(simpleBackup.open(QFile::ReadOnly) &&
                simpleBackup.readAll() == QByteArray("previous") &&
                backupPath.endsWith(QStringLiteral(".bak")),
                "Simple backup content or name is wrong");
    ok &= check(FileManager::backupFileBeforeSave(
                    backupSource, 2, false, QString(),
                    QStringLiteral("2026-07-26_120000"),
                    &backupPath, &backupError) &&
                QFileInfo(backupPath).absolutePath().endsWith(
                    QStringLiteral("nppBackup")) &&
                backupPath.endsWith(
                    QStringLiteral(".2026-07-26_120000.bak")),
                "Verbose backup path is wrong");

    NppGUI& gui = NppParameters::getInstance().getNppGUI();
    gui._autoInsertParentheses = true;
    gui._urlMode = 2;
    gui._enableTagsMatchHighlight = true;
    ScintillaEditView behaviorView;
    behaviorView.setText(QStringLiteral("needle"));
    behaviorView.setSelection(0, 0, 0, 6);
    ok &= check(behaviorView.selectedText() == QStringLiteral("needle"),
                "SCI_GETSELTEXT wrapper truncated the selection");
    behaviorView.setText(QStringLiteral("("));
    behaviorView.setCurrentPositionNpp(1);
    ok &= check(QMetaObject::invokeMethod(
                    &behaviorView, "onCharacterAdded",
                    Q_ARG(int, static_cast<int>('('))) &&
                behaviorView.text() == QStringLiteral("()"),
                "Parenthesis auto-pair did not insert the closer");
    behaviorView.setText(QStringLiteral("see https://example.com now"));
    behaviorView.refreshUrlHotspots();
    const int urlMask = static_cast<int>(behaviorView.SendScintillaNpp(
        SCI_INDICATORALLONFOR, 6));
    ok &= check((urlMask & (1 << 27)) != 0,
                "URL hotspot indicator was not applied");
    behaviorView.setLexerForFile(QStringLiteral("sample.xml"));
    ok &= check(
        behaviorView.hasLexer(),
        "Built-in language did not install a Lexilla lexer instance");
    ok &= check(lexerProperty(behaviorView, "lexer.xml.allow.scripts") == "0" &&
                lexerProperty(behaviorView, "fold.html") == "1",
                "XML lexer properties do not match the v8.4.6 setup");
    behaviorView.setText(QStringLiteral("<root><child/></root>"));
    behaviorView.SendScintilla(SCI_COLOURISE, 0, -1);
    ok &= check(
        behaviorView.SendScintillaNpp(SCI_GETSTYLEAT, 2, 0) == SCE_H_TAG,
        "Independent Lexilla XML lexer did not style the document");
    behaviorView.setCurrentPositionNpp(2);
    behaviorView.refreshXmlTagHighlight();
    const int openTagMask = static_cast<int>(behaviorView.SendScintillaNpp(
        SCI_INDICATORALLONFOR, 1));
    const int closeTagMask = static_cast<int>(behaviorView.SendScintillaNpp(
        SCI_INDICATORALLONFOR, 16));
    ok &= check((openTagMask & (1 << 28)) != 0 &&
                (closeTagMask & (1 << 28)) != 0,
                "XML matching tag indicators were not applied");

    ok &= check(behaviorView.setLexerByName(QStringLiteral("cpp")),
                "C++ lexer could not be installed");
    ok &= check(behaviorView.lexerKeywordSet(0).contains(
                    QStringLiteral("alignof")) &&
                behaviorView.lexerKeywordSet(1).contains(
                    QStringLiteral("constexpr")) &&
                behaviorView.lexerKeywordSet(2).contains(
                    QStringLiteral("param")) &&
                lexerProperty(behaviorView,
                    "lexer.cpp.track.preprocessor") == "0",
                "C++ keyword slot mapping or properties differ from v8.4.6");

    ok &= check(behaviorView.setLexerByName(QStringLiteral("html")),
                "HTML lexer could not be installed");
    ok &= check(behaviorView.lexerKeywordSet(0).contains(
                    QStringLiteral("html")) &&
                !behaviorView.lexerKeywordSet(1).isEmpty() &&
                behaviorView.lexerKeywordSet(2).contains(
                    QStringLiteral("function")) &&
                behaviorView.lexerKeywordSet(4).contains(
                    QStringLiteral("foreach")),
                "HTML embedded lexer keyword slots differ from v8.4.6");

    ok &= check(behaviorView.setLexerByName(QStringLiteral("baanc")),
                "BaanC lexer could not be installed");
    ok &= check(!behaviorView.lexerKeywordSet(8).isEmpty() &&
                lexerProperty(behaviorView,
                    "fold.baan.inner.level") == "1",
                "High keyword slots or BaanC properties were not applied");

    ScintillaEditView externalLexerView;
    ok &= check(externalLexerView.installExternalLexer(
                    QStringLiteral("cpp"), &CreateLexer) &&
                externalLexerView.hasLexer(),
                "External ILexer5 factory interface could not install a lexer");
    behaviorView.setText(QStringLiteral("alpha beta"));
    behaviorView.SendScintilla(
        SCI_SETINDICATORCURRENT,
        ScintillaEditView::FIND_MARK_INDICATOR);
    behaviorView.SendScintilla(
        SCI_INDICATORFILLRANGE, 0, 5);
    behaviorView.SendScintilla(
        SCI_INDICATORFILLRANGE, 6, 4);
    ok &= check(
        behaviorView.markedText(
            ScintillaEditView::FIND_MARK_INDICATOR)
            == QStringLiteral("alpha\r\nbeta\r\n"),
        "Copy Marked Text did not preserve indicator ranges");

    UserLangDesc userLanguage;
    userLanguage.name = QStringLiteral("P1 UDL");
    userLanguage.keywordLists[19] = QStringLiteral("alpha");
    WordsStyle keywordStyle;
    keywordStyle.name = QStringLiteral("KEYWORDS1");
    keywordStyle.styleID = 4;
    keywordStyle.fgColor = QColor(Qt::red);
    keywordStyle.hasFg = true;
    userLanguage.styles.append(keywordStyle);
    behaviorView.setUserDefinedLanguage(userLanguage);
    behaviorView.setText(QStringLiteral("alpha beta"));
    behaviorView.SendScintilla(SCI_COLOURISE, 0, -1);
    ok &= check(
        behaviorView.SendScintillaNpp(SCI_GETSTYLEAT, 1, 0) == 4 &&
        behaviorView.SendScintillaNpp(SCI_GETSTYLEAT, 7, 0) == 0,
        "Original LexUser engine did not apply UDL keyword semantics");

    return ok ? 0 : 1;
}
