// ScintillaEditView.cpp - Scintilla 编辑器视图实现
// 移植自: v8.4.6:PowerEditor/src/ScintillaComponent/ScintillaEditView.cpp

#include "ScintillaEditView.h"
#include "UserDefinedLexer.h"
#include "../Parameters.h"
#include "AutoCompletionParser.h"
#include <ILexer.h>
#include <Lexilla.h>
#include <QFileInfo>
#include <QFont>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>

namespace {

struct LexerNameMapping
{
    const char* nppName;
    const char* scintillaName;
};

// v8.4.6 ScintillaEditView::_langNameInfoArray, excluding normal, UDL and
// external lexers. Keeping this table independent from the UI also makes
// command-line and automatic extension selection use the same mapping.
const LexerNameMapping lexerNameMappings[] = {
    {"php", "phpscript"}, {"c", "cpp"}, {"cpp", "cpp"},
    {"cs", "cpp"}, {"objc", "objc"}, {"java", "cpp"}, {"rc", "cpp"},
    {"html", "hypertext"}, {"xml", "xml"}, {"makefile", "makefile"},
    {"pascal", "pascal"}, {"batch", "batch"}, {"ini", "props"},
    {"asp", "hypertext"}, {"sql", "sql"}, {"vb", "vb"},
    {"javascript", "cpp"}, {"css", "css"}, {"perl", "perl"},
    {"python", "python"}, {"lua", "lua"}, {"tex", "tex"},
    {"fortran", "fortran"}, {"bash", "bash"}, {"actionscript", "cpp"},
    {"nsis", "nsis"}, {"tcl", "tcl"}, {"lisp", "lisp"},
    {"scheme", "lisp"}, {"asm", "asm"}, {"diff", "diff"},
    {"props", "props"}, {"postscript", "ps"}, {"ruby", "ruby"},
    {"smalltalk", "smalltalk"}, {"vhdl", "vhdl"}, {"kix", "kix"},
    {"autoit", "au3"}, {"caml", "caml"}, {"ada", "ada"},
    {"verilog", "verilog"}, {"matlab", "matlab"},
    {"haskell", "haskell"}, {"inno", "inno"}, {"cmake", "cmake"},
    {"yaml", "yaml"}, {"cobol", "COBOL"}, {"gui4cli", "gui4cli"},
    {"d", "d"}, {"powershell", "powershell"}, {"r", "r"},
    {"jsp", "hypertext"}, {"coffeescript", "coffeescript"},
    {"json", "json"}, {"javascript.js", "cpp"}, {"fortran77", "f77"},
    {"baanc", "baan"}, {"srec", "srec"}, {"ihex", "ihex"},
    {"tehex", "tehex"}, {"swift", "cpp"}, {"asn1", "asn1"},
    {"avs", "avs"}, {"blitzbasic", "blitzbasic"},
    {"purebasic", "purebasic"}, {"freebasic", "freebasic"},
    {"csound", "csound"}, {"erlang", "erlang"}, {"escript", "escript"},
    {"forth", "forth"}, {"latex", "latex"}, {"mmixal", "mmixal"},
    {"nim", "nimrod"}, {"nncrontab", "nncrontab"},
    {"oscript", "oscript"}, {"rebol", "rebol"}, {"registry", "registry"},
    {"rust", "rust"}, {"spice", "spice"}, {"txt2tags", "txt2tags"},
    {"visualprolog", "visualprolog"}, {"typescript", "cpp"},
    {"markdown", "markdown"}, {"searchresult", "errorlist"}
};

struct LanguageKeywordMask
{
    const char* name;
    quint16 mask;
};

constexpr quint16 keywordMask(std::initializer_list<int> indices)
{
    quint16 mask = 0;
    for (const int slot : indices)
        mask |= static_cast<quint16>(1u << slot);
    return mask;
}

// Mirrors the LIST_* selections used by v8.4.6's simple lexer setters.
const LanguageKeywordMask languageKeywordMasks[] = {
    {"css", keywordMask({0, 1, 4, 6})},
    {"lua", keywordMask({0, 1, 2, 3})},
    {"makefile", 0}, {"ini", 0},
    {"sql", keywordMask({0, 1, 4})}, {"bash", keywordMask({0})},
    {"vb", keywordMask({0})}, {"pascal", keywordMask({0})},
    {"perl", keywordMask({0})}, {"python", keywordMask({0, 1})},
    {"batch", keywordMask({0})}, {"tex", 0},
    {"nsis", keywordMask({0, 1, 2, 3})},
    {"fortran", keywordMask({0, 1, 2})},
    {"fortran77", keywordMask({0, 1, 2})},
    {"lisp", keywordMask({0, 1})}, {"scheme", keywordMask({0, 1})},
    {"asm", keywordMask({0, 1, 2, 3, 4, 5})},
    {"diff", 0}, {"props", 0},
    {"postscript", keywordMask({0, 1, 2, 3})},
    {"ruby", keywordMask({0})}, {"smalltalk", keywordMask({0})},
    {"vhdl", keywordMask({0, 1, 2, 3, 4, 5, 6})},
    {"kix", keywordMask({0, 1, 2})},
    {"autoit", keywordMask({0, 1, 2, 3, 4, 5, 6})},
    {"caml", keywordMask({0, 1, 2})}, {"ada", keywordMask({0})},
    {"verilog", keywordMask({0, 1})}, {"matlab", keywordMask({0})},
    {"haskell", keywordMask({0})},
    {"inno", keywordMask({0, 1, 2, 3, 4, 5})},
    {"cmake", keywordMask({0, 1, 2})}, {"yaml", keywordMask({0})},
    {"cobol", keywordMask({0, 1, 2})},
    {"gui4cli", keywordMask({0, 1, 2, 3, 4})},
    {"d", keywordMask({0, 1, 2, 3, 4, 5, 6})},
    {"powershell", keywordMask({0, 1, 2, 5})},
    {"r", keywordMask({0, 1, 2})},
    {"coffeescript", keywordMask({0, 1, 2, 3})},
    {"baanc", keywordMask({0, 1, 2, 3, 4, 5, 6, 7, 8})},
    {"srec", 0}, {"ihex", 0}, {"tehex", 0},
    {"asn1", keywordMask({0, 1, 2, 3})},
    {"avs", keywordMask({0, 1, 2, 3, 4, 5})},
    {"blitzbasic", keywordMask({0, 1, 2, 3})},
    {"purebasic", keywordMask({0, 1, 2, 3})},
    {"freebasic", keywordMask({0, 1, 2, 3})},
    {"csound", keywordMask({0, 1, 2})},
    {"erlang", keywordMask({0, 1, 2, 3, 4, 5})},
    {"escript", keywordMask({0, 1, 2})},
    {"forth", keywordMask({0, 1, 2, 3, 4, 5})},
    {"latex", 0}, {"mmixal", keywordMask({0, 1, 2})},
    {"nim", keywordMask({0})},
    {"nncrontab", keywordMask({0, 1, 2})},
    {"oscript", keywordMask({0, 1, 2, 3, 4, 5})},
    {"rebol", keywordMask({0, 1, 2, 3, 4, 5, 6})},
    {"registry", 0}, {"rust", keywordMask({0, 1, 2, 3, 4, 5, 6})},
    {"spice", keywordMask({0, 1, 2})}, {"txt2tags", 0},
    {"visualprolog", keywordMask({0, 1, 2, 3})},
    {"searchresult", 0}
};

int keywordClassIndex(const QString& keywordClass)
{
    const QString normalized = keywordClass.trimmed().toLower();
    if (normalized == QStringLiteral("instre1")) return 0;
    if (normalized == QStringLiteral("instre2")) return 1;
    const QRegularExpressionMatch match =
        QRegularExpression(QStringLiteral("^type([1-7])$")).match(normalized);
    return match.hasMatch() ? match.captured(1).toInt() + 1 : -1;
}

} // namespace

// 静态成员初始化
const int ScintillaEditView::_SC_MARGE_LINENUMBER;
const int ScintillaEditView::_SC_MARGE_SYMBOL;
const int ScintillaEditView::_SC_MARGE_FOLDER;
const int ScintillaEditView::URL_INDICATOR;
const int ScintillaEditView::XML_TAG_INDICATOR;

ScintillaEditView::ScintillaEditView(QWidget *parent)
    : ScintillaEditBase(parent)
{
    // 初始化编辑器
    init();
}

ScintillaEditView::~ScintillaEditView()
{
    // Qt 会自动清理
}

QString ScintillaEditView::text(int line) const
{
    const sptr_t length = send(SCI_LINELENGTH, line);
    QByteArray bytes(static_cast<int>(length) + 1, '\0');
    send(SCI_GETLINE, line, reinterpret_cast<sptr_t>(bytes.data()));
    bytes.truncate(static_cast<int>(length));
    return QString::fromUtf8(bytes);
}

QString ScintillaEditView::text(int start, int end) const
{
    if (end <= start)
        return QString();
    QByteArray bytes(end - start + 1, '\0');
    Sci_TextRange range{{static_cast<Sci_PositionCR>(start),
                         static_cast<Sci_PositionCR>(end)}, bytes.data()};
    send(SCI_GETTEXTRANGE, 0, reinterpret_cast<sptr_t>(&range));
    bytes.truncate(end - start);
    return QString::fromUtf8(bytes);
}

QString ScintillaEditView::selectedText() const
{
    const sptr_t length = send(SCI_GETSELTEXT);
    if (length <= 0)
        return QString();
    QByteArray bytes(static_cast<int>(length) + 1, '\0');
    send(SCI_GETSELTEXT, 0, reinterpret_cast<sptr_t>(bytes.data()));
    bytes.truncate(static_cast<int>(length));
    return QString::fromUtf8(bytes);
}

void ScintillaEditView::replaceSelectedText(const QString& text)
{
    const QByteArray utf8 = text.toUtf8();
    send(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(utf8.constData()));
}

void ScintillaEditView::insert(const QString& text)
{
    const QByteArray utf8 = text.toUtf8();
    send(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(utf8.constData()));
}

void ScintillaEditView::insertAt(const QString& text, int line, int index)
{
    const sptr_t lineStart = send(SCI_POSITIONFROMLINE, line);
    const sptr_t position = send(SCI_POSITIONRELATIVE, lineStart, index);
    const QByteArray utf8 = text.toUtf8();
    send(SCI_INSERTTEXT, position, reinterpret_cast<sptr_t>(utf8.constData()));
}

void ScintillaEditView::setCursorPosition(int line, int index)
{
    const sptr_t position = send(SCI_FINDCOLUMN, line, index);
    send(SCI_GOTOPOS, position);
}

void ScintillaEditView::getCursorPosition(int* line, int* index) const
{
    const sptr_t position = send(SCI_GETCURRENTPOS);
    const int currentLine = static_cast<int>(send(SCI_LINEFROMPOSITION, position));
    if (line) *line = currentLine;
    if (index) *index = static_cast<int>(send(SCI_GETCOLUMN, position));
}

void ScintillaEditView::setSelection(int lineFrom, int indexFrom,
                                     int lineTo, int indexTo)
{
    send(SCI_SETSEL, send(SCI_FINDCOLUMN, lineFrom, indexFrom),
         send(SCI_FINDCOLUMN, lineTo, indexTo));
}

void ScintillaEditView::getSelection(int* lineFrom, int* indexFrom,
                                     int* lineTo, int* indexTo) const
{
    const sptr_t start = send(SCI_GETSELECTIONSTART);
    const sptr_t end = send(SCI_GETSELECTIONEND);
    const int startLine = static_cast<int>(send(SCI_LINEFROMPOSITION, start));
    const int endLine = static_cast<int>(send(SCI_LINEFROMPOSITION, end));
    if (lineFrom) *lineFrom = startLine;
    if (indexFrom) *indexFrom = static_cast<int>(send(SCI_GETCOLUMN, start));
    if (lineTo) *lineTo = endLine;
    if (indexTo) *indexTo = static_cast<int>(send(SCI_GETCOLUMN, end));
}

void ScintillaEditView::setMarginWidth(int margin, const QString& sample)
{
    const QByteArray utf8 = sample.toUtf8();
    send(SCI_SETMARGINWIDTHN, margin,
         send(SCI_TEXTWIDTH, STYLE_LINENUMBER,
              reinterpret_cast<sptr_t>(utf8.constData())));
}

void ScintillaEditView::setFont(const QFont& font)
{
    QWidget::setFont(font);
    const QByteArray family = font.family().toUtf8();
    send(SCI_STYLESETFONT, STYLE_DEFAULT,
         reinterpret_cast<sptr_t>(family.constData()));
    send(SCI_STYLESETSIZE, STYLE_DEFAULT, font.pointSize());
    send(SCI_STYLECLEARALL);
}

void ScintillaEditView::lineIndexFromPosition(int position, int* line,
                                               int* index) const
{
    const int currentLine = static_cast<int>(send(SCI_LINEFROMPOSITION, position));
    if (line) *line = currentLine;
    if (index) *index = static_cast<int>(send(SCI_GETCOLUMN, position));
}

QString ScintillaEditView::wordAtLineIndex(int line, int index) const
{
    const sptr_t position = send(SCI_FINDCOLUMN, line, index);
    const sptr_t start = send(SCI_WORDSTARTPOSITION, position, 1);
    const sptr_t end = send(SCI_WORDENDPOSITION, position, 1);
    if (end <= start)
        return QString();
    QByteArray bytes(static_cast<int>(end - start) + 1, '\0');
    Sci_TextRange range{{static_cast<Sci_PositionCR>(start),
                         static_cast<Sci_PositionCR>(end)}, bytes.data()};
    send(SCI_GETTEXTRANGE, 0, reinterpret_cast<sptr_t>(&range));
    bytes.truncate(static_cast<int>(end - start));
    return QString::fromUtf8(bytes);
}

void ScintillaEditView::setFolding(int)
{
    send(SCI_SETPROPERTY, reinterpret_cast<uptr_t>("fold"),
         reinterpret_cast<sptr_t>("1"));
    send(SCI_SETMARGINMASKN, _SC_MARGE_FOLDER, SC_MASK_FOLDERS);
    send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
    send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
    send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
    send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
    send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
    send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
    send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);
}

void ScintillaEditView::autoCompleteFromAPIs()
{
    if (_completionWords.isEmpty())
        return;
    const sptr_t current = send(SCI_GETCURRENTPOS);
    const sptr_t start = send(SCI_WORDSTARTPOSITION, current, 1);
    send(SCI_AUTOCSETSEPARATOR, '\n');
    const QByteArray list = _completionWords.join(QLatin1Char('\n')).toUtf8();
    send(SCI_AUTOCSHOW, current - start,
         reinterpret_cast<sptr_t>(list.constData()));
}

void ScintillaEditView::autoCompleteFromDocument()
{
    const QRegularExpression words(QStringLiteral("[A-Za-z_][A-Za-z0-9_]*"));
    QStringList entries;
    auto match = words.globalMatch(getText());
    while (match.hasNext())
        entries.append(match.next().captured());
    entries.removeDuplicates();
    entries.sort(Qt::CaseInsensitive);
    if (entries.isEmpty())
        return;
    const sptr_t current = send(SCI_GETCURRENTPOS);
    const sptr_t start = send(SCI_WORDSTARTPOSITION, current, 1);
    send(SCI_AUTOCSETSEPARATOR, '\n');
    const QByteArray list = entries.join(QLatin1Char('\n')).toUtf8();
    send(SCI_AUTOCSHOW, current - start,
         reinterpret_cast<sptr_t>(list.constData()));
}

void ScintillaEditView::callTip()
{
    if (_completionCallTips.isEmpty())
        return;

    const qintptr current = send(SCI_GETCURRENTPOS);
    const qintptr rangeStart = qMax<qintptr>(0, current - 256);
    const QString prefix = text(static_cast<int>(rangeStart),
                                static_cast<int>(current));
    const QRegularExpression identifier(
        QStringLiteral("([A-Za-z_][A-Za-z0-9_:.]*)\\s*(?:\\([^()]*)?$"));
    const QRegularExpressionMatch match = identifier.match(prefix);
    if (!match.hasMatch())
        return;

    const QString name = match.captured(1);
    QStringList matches;
    for (const QString& signature : _completionCallTips) {
        if (signature.startsWith(name + QLatin1Char('('),
                                 Qt::CaseInsensitive))
            matches.append(signature);
    }
    if (matches.isEmpty())
        return;

    const QByteArray callTips = matches.join(QLatin1Char('\n')).toUtf8();
    send(SCI_CALLTIPSHOW, current,
         reinterpret_cast<sptr_t>(callTips.constData()));
}

bool ScintillaEditView::createLargeDocument()
{
    const sptr_t created = send(SCI_CREATEDOCUMENT, 0,
        SC_DOCUMENTOPTION_STYLES_NONE | SC_DOCUMENTOPTION_TEXT_LARGE);
    setDocument(created);
    send(SCI_RELEASEDOCUMENT, 0, created);
    const qintptr options = send(SCI_GETDOCUMENTOPTIONS);
    return (options & SC_DOCUMENTOPTION_STYLES_NONE) &&
           (options & SC_DOCUMENTOPTION_TEXT_LARGE);
}

void ScintillaEditView::createStandardDocument()
{
    const sptr_t created = send(SCI_CREATEDOCUMENT);
    setDocument(created);
    send(SCI_RELEASEDOCUMENT, 0, created);
}

qintptr ScintillaEditView::documentLengthNpp() const
{
    return SendScintillaNpp(SCI_GETLENGTH);
}

bool ScintillaEditView::appendUtf8Chunk(const QByteArray& bytes)
{
    if (bytes.isEmpty())
        return true;
    SendScintillaNpp(
        SCI_APPENDTEXT, static_cast<quintptr>(bytes.size()),
        reinterpret_cast<qintptr>(bytes.constData()));
    const qintptr status = SendScintillaNpp(SCI_GETSTATUS);
    return status == SC_STATUS_OK || status >= SC_STATUS_WARN_START;
}

const char* ScintillaEditView::utf8RangePointer(
    qintptr position, qintptr length) const
{
    return reinterpret_cast<const char*>(SendScintillaPtrResult(
        SCI_GETRANGEPOINTER, static_cast<quintptr>(position), length));
}

qintptr ScintillaEditView::gapPositionNpp() const
{
    return SendScintillaNpp(SCI_GETGAPPOSITION);
}

void ScintillaEditView::beginBulkLoad(qint64 expectedBytes)
{
    setReadOnly(false);
    SendScintillaNpp(SCI_SETSTATUS, SC_STATUS_OK);
    SendScintillaNpp(SCI_SETUNDOCOLLECTION, 0);
    SendScintillaNpp(SCI_CLEARALL);
    if (expectedBytes > 0) {
        const qint64 editingRoom =
            qMin<qint64>(qint64(1) << 20, expectedBytes / 6);
        SendScintillaNpp(
            SCI_ALLOCATE,
            static_cast<quintptr>(expectedBytes + editingRoom));
    }
}

bool ScintillaEditView::endBulkLoad()
{
    SendScintillaNpp(SCI_SETUNDOCOLLECTION, 1);
    SendScintillaNpp(SCI_EMPTYUNDOBUFFER);
    SendScintillaNpp(SCI_SETSAVEPOINT);
    const qintptr status = SendScintillaNpp(SCI_GETSTATUS);
    return status == SC_STATUS_OK || status >= SC_STATUS_WARN_START;
}

void ScintillaEditView::setLargeFileMode(bool enabled)
{
    _largeFileMode = enabled;
    if (enabled) {
        clearLexer();
        setBraceMatching(NoBraceMatch);
        setWrapMode(WrapNone);
        setAutoCompletionSource(AcsNone);
        setAutoCompletionThreshold(-1);
    } else {
        setBraceMatching(SloppyBraceMatch);
    }
}

void ScintillaEditView::init()
{
    // QAbstractScrollArea owns a separate viewport.  Scintilla changes the
    // cursor while moving across margins, so the viewport must receive moves
    // even when no mouse button is pressed.
    viewport()->setMouseTracking(true);

    _urlRefreshTimer = new QTimer(this);
    _urlRefreshTimer->setSingleShot(true);
    _urlRefreshTimer->setInterval(120);
    connect(_urlRefreshTimer, &QTimer::timeout,
            this, &ScintillaEditView::refreshUrlHotspots);
    // 设置默认样式
    setupDefaultStyles();

    // 设置边距
    setupMargins();

    // 设置指示器
    setupIndicators();

    // 设置基本属性
    execute(SCI_SETCODEPAGE, SC_CP_UTF8);
    setEolMode(EolWindows);  // Windows 行尾

    // 设置制表符
    setTabWidth(4);
    setIndentationsUseTabs(true);

    // 设置自动缩进
    setAutoIndent(true);

    // 设置括号匹配
    setBraceMatching(SloppyBraceMatch);

    // 边距点击（切换书签）
    connect(this, &ScintillaEditBase::marginClicked, this,
            [this](Scintilla::Position position, Scintilla::KeyMod modifiers,
                   int margin) {
        onMarginClicked(margin,
            static_cast<int>(send(SCI_LINEFROMPOSITION, position)),
            static_cast<Qt::KeyboardModifiers>(static_cast<int>(modifiers)));
    });

    // Smart Highlighting：通过 SCN_UPDATEUI 驱动（选区变化/滚动均触发）
    connect(this, &ScintillaEditBase::updateUi, this,
            [this](Scintilla::Update updated) {
        updateSmartHighlight(static_cast<int>(updated));
        int line = 0, index = 0;
        getCursorPosition(&line, &index);
        emit cursorPositionChanged(line, index);
    });
    connect(this, &ScintillaEditBase::charAdded,
            this, &ScintillaEditView::onCharacterAdded);
    connect(this, &ScintillaEditBase::notify, this,
            [this](Scintilla::NotificationData* notification) {
        if (notification && notification->nmhdr.code ==
                Scintilla::Notification::IndicatorRelease) {
            onIndicatorClicked(
                static_cast<int>(notification->position),
                static_cast<int>(notification->modifiers));
        }
    });
    connect(this, &ScintillaEditBase::modified, this,
            [this](Scintilla::ModificationFlags type, Scintilla::Position,
                   Scintilla::Position, Scintilla::Position,
                   const QByteArray&, Scintilla::Position,
                   Scintilla::FoldLevel, Scintilla::FoldLevel) {
        if ((static_cast<int>(type) & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) != 0)
            emit textChanged();
    });
    connect(this, &textChanged,
            this, &ScintillaEditView::scheduleUrlRefresh);
    connect(this, &cursorPositionChanged,
            this, [this](int, int) { refreshXmlTagHighlight(); });

    // 当前行背景高亮（对应原版 SCI_SETCARETLINEVISIBLEALWAYS）
    // 颜色由 applyGlobalStyles() 从 stylers.xml "Current line background colour" 读取
    setCaretLineVisible(true);
    execute(SCI_SETCARETLINEVISIBLEALWAYS, 1);  // 失焦时仍保持高亮

    // 默认无语法高亮（新建文件为纯文本）

    // 应用全局样式（选区颜色等），stylers.xml 由 NppParameters::load() 预先加载
    applyGlobalStyles();
}

void ScintillaEditView::setBorderEdge(bool enabled, bool darkMode)
{
    if (!enabled) {
        setFrameStyle(QFrame::NoFrame);
        return;
    }

    setLineWidth(1);
    setMidLineWidth(0);
    setFrameStyle(darkMode
        ? QFrame::Box | QFrame::Plain
        : QFrame::WinPanel | QFrame::Sunken);
}

void ScintillaEditView::setupDefaultStyles()
{
    // 设置默认字体
    QFont font("Consolas", 10);
    setFont(font);

    // 选择背景色由 applyGlobalStyles() 从 stylers.xml 加载（"Selected text colour"）

    // 设置插入符颜色
    setCaretForegroundColor(QColor(0, 0, 0));
    setCaretWidth(1);
}

void ScintillaEditView::setupMargins()
{
    // 行号边距
    setMarginType(_SC_MARGE_LINENUMBER, NumberMargin);
    setMarginWidth(_SC_MARGE_LINENUMBER, "00000");
    setMarginsForegroundColor(QColor(128, 128, 128));
    setMarginsBackgroundColor(QColor(240, 240, 240));

    // 符号边距（书签等）
    // Notepad++ uses a colour margin so "Bookmark margin" from stylers.xml
    // controls the whole strip independently from the line-number style.
    setMarginType(_SC_MARGE_SYMBOL, ColourMargin);
    setMarginWidth(_SC_MARGE_SYMBOL, 16);
    execute(SCI_SETMARGINMASKN, _SC_MARGE_SYMBOL, 1 << BOOKMARK_MARKER);
    setMarginSensitivity(_SC_MARGE_SYMBOL, true);
    execute(SCI_SETMARGINCURSORN, _SC_MARGE_SYMBOL, SC_CURSORREVERSEARROW);

    // 书签标记：蓝色圆形
    markerDefine(Circle, BOOKMARK_MARKER);
    setMarkerBackgroundColor(QColor(50, 130, 255), BOOKMARK_MARKER);
    setMarkerForegroundColor(QColor(255, 255, 255), BOOKMARK_MARKER);

    // 折叠边距
    setMarginType(_SC_MARGE_FOLDER, SymbolMargin);
    setMarginWidth(_SC_MARGE_FOLDER, 14);
    execute(SCI_SETMARGINMASKN, _SC_MARGE_FOLDER, SC_MASK_FOLDERS);
    setFolding(BoxedTreeFoldStyle);
    setMarginSensitivity(_SC_MARGE_FOLDER, true);
    execute(SCI_SETMARGINCURSORN, _SC_MARGE_FOLDER, SC_CURSORARROW);
}

void ScintillaEditView::setupIndicators()
{
    // 查找高亮指示器（indicator 0）
    execute(SCI_INDICSETSTYLE, 0, INDIC_ROUNDBOX);
    execute(SCI_INDICSETALPHA, 0, 100);
    execute(SCI_INDICSETUNDER, 0, true);

    // Smart Highlighting 指示器（indicator 29，对应原版 SCE_UNIVERSAL_FOUND_STYLE_SMART）
    // 颜色由 applyGlobalStyles() 从 stylers.xml "Smart Highlighting" bgColor 覆盖
    execute(SCI_INDICSETSTYLE, SMART_HIGHLIGHT_INDICATOR, INDIC_ROUNDBOX);
    execute(SCI_INDICSETALPHA, SMART_HIGHLIGHT_INDICATOR, 100);
    execute(SCI_INDICSETUNDER, SMART_HIGHLIGHT_INDICATOR, true);
    execute(SCI_INDICSETFORE,  SMART_HIGHLIGHT_INDICATOR, 0x00FF00);  // 默认绿色（BGR）

    // Find All / Mark All 高亮指示器（indicator 31，对应原版 SCE_UNIVERSAL_FOUND_STYLE）
    // 颜色由 applyGlobalStyles() 从 stylers.xml "Find Mark Style" bgColor 覆盖
    execute(SCI_INDICSETSTYLE, FIND_MARK_INDICATOR, INDIC_ROUNDBOX);
    execute(SCI_INDICSETALPHA, FIND_MARK_INDICATOR, 100);
    execute(SCI_INDICSETUNDER, FIND_MARK_INDICATOR, true);
    execute(SCI_INDICSETFORE,  FIND_MARK_INDICATOR, 0x0000FF);  // 默认蓝色（BGR）
    execute(SCI_INDICSETSTYLE, URL_INDICATOR, INDIC_PLAIN);
    execute(SCI_INDICSETHOVERSTYLE, URL_INDICATOR, INDIC_PLAIN);
    execute(SCI_INDICSETFORE, URL_INDICATOR, 0xFF0000);
    execute(SCI_INDICSETUNDER, URL_INDICATOR, true);
    execute(SCI_INDICSETSTYLE, XML_TAG_INDICATOR, INDIC_ROUNDBOX);
    execute(SCI_INDICSETALPHA, XML_TAG_INDICATOR, 80);
    execute(SCI_INDICSETFORE, XML_TAG_INDICATOR, 0x00A5FF);
    execute(SCI_INDICSETUNDER, XML_TAG_INDICATOR, true);
}

// 文本操作方法
void ScintillaEditView::setText(const QString &text)
{
    const QByteArray utf8 = text.toUtf8();
    send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(utf8.constData()));
}

QString ScintillaEditView::getText() const
{
    const sptr_t length = send(SCI_GETLENGTH);
    QByteArray utf8(static_cast<int>(length) + 1, '\0');
    send(SCI_GETTEXT, length + 1, reinterpret_cast<sptr_t>(utf8.data()));
    utf8.truncate(static_cast<int>(length));
    return QString::fromUtf8(utf8);
}

void ScintillaEditView::appendText(const QString &text)
{
    const QByteArray utf8 = text.toUtf8();
    send(SCI_APPENDTEXT, utf8.size(),
         reinterpret_cast<sptr_t>(utf8.constData()));
}

void ScintillaEditView::insertText(int pos, const QString &text)
{
    const QByteArray utf8 = text.toUtf8();
    send(SCI_INSERTTEXT, pos, reinterpret_cast<sptr_t>(utf8.constData()));
}

// 光标和选择方法
int ScintillaEditView::getCurrentPos() const
{
    return execute(SCI_GETCURRENTPOS);
}

void ScintillaEditView::setCurrentPos(int pos)
{
    execute(SCI_SETCURRENTPOS, pos);
}

void ScintillaEditView::getSelection(int *startPos, int *endPos) const
{
    if (startPos) {
        *startPos = execute(SCI_GETSELECTIONSTART);
    }
    if (endPos) {
        *endPos = execute(SCI_GETSELECTIONEND);
    }
}

qintptr ScintillaEditView::currentPositionNpp() const
{
    return SendScintillaNpp(SCI_GETCURRENTPOS);
}

void ScintillaEditView::setCurrentPositionNpp(qintptr position)
{
    SendScintillaNpp(
        SCI_GOTOPOS, static_cast<quintptr>(qMax<qintptr>(0, position)));
}

void ScintillaEditView::getSelectionNpp(
    qintptr* startPos, qintptr* endPos) const
{
    if (startPos)
        *startPos = SendScintillaNpp(SCI_GETSELECTIONSTART);
    if (endPos)
        *endPos = SendScintillaNpp(SCI_GETSELECTIONEND);
}

bool ScintillaEditView::findFirst(
    const QString& expression, bool regex, bool matchCase, bool wholeWord,
    bool wrap, bool forward, int line, int index, bool show,
    bool posix, bool cxx11)
{
    if (expression.isEmpty()) {
        _searchState.active = false;
        return false;
    }

    _searchState.expression = expression;
    unsigned int flags = 0;
    if (matchCase)
        flags |= SCFIND_MATCHCASE;
    if (wholeWord)
        flags |= SCFIND_WHOLEWORD;
    if (regex) {
        flags |= SCFIND_REGEXP;
        flags |= SCFIND_REGEXP_EMPTYMATCH_ALL;
        flags |= SCFIND_REGEXP_SKIPCRLFASONE;
        if (_searchState.expression.startsWith(QStringLiteral("(?s)"))) {
            _searchState.expression.remove(0, 4);
            flags |= SCFIND_REGEXP_DOTMATCHESNL;
        }
    }
    if (posix)
        flags |= SCFIND_POSIX;
    if (cxx11)
        flags |= SCFIND_CXX11REGEX;

    _searchState.flags = static_cast<int>(flags);
    _searchState.wrap = wrap;
    _searchState.forward = forward;
    _searchState.show = show;
    _searchState.active = true;
    if (line < 0 || index < 0)
        _searchState.start = SendScintillaNpp(SCI_GETCURRENTPOS);
    else
        _searchState.start = positionFromLineIndex(line, index);
    _searchState.end = forward ? SendScintillaNpp(SCI_GETLENGTH) : 0;
    return doNppFind();
}

bool ScintillaEditView::findNext()
{
    return _searchState.active && doNppFind();
}

bool ScintillaEditView::doNppFind()
{
    const QByteArray pattern = _searchState.expression.toUtf8();
    auto searchRange = [this, &pattern]() {
        SendScintilla(SCI_SETSEARCHFLAGS, _searchState.flags);
        SendScintillaNpp(
            SCI_SETTARGETSTART,
            static_cast<quintptr>(_searchState.start));
        SendScintillaNpp(
            SCI_SETTARGETEND,
            static_cast<quintptr>(_searchState.end));
        return SendScintillaNpp(
            SCI_SEARCHINTARGET, static_cast<quintptr>(pattern.size()),
            reinterpret_cast<qintptr>(pattern.constData()));
    };

    qintptr found = searchRange();
    if (found < 0 && _searchState.wrap) {
        _searchState.start = _searchState.forward
            ? 0
            : SendScintillaNpp(SCI_GETLENGTH);
        _searchState.end = _searchState.forward
            ? SendScintillaNpp(SCI_GETLENGTH)
            : 0;
        found = searchRange();
    }
    if (found < 0) {
        _searchState.active = false;
        return false;
    }

    const qintptr targetStart = SendScintillaNpp(SCI_GETTARGETSTART);
    const qintptr targetEnd = SendScintillaNpp(SCI_GETTARGETEND);
    if (_searchState.show) {
        const int firstLine = static_cast<int>(
            SendScintillaNpp(
                SCI_LINEFROMPOSITION,
                static_cast<quintptr>(targetStart)));
        const int lastLine = static_cast<int>(
            SendScintillaNpp(
                SCI_LINEFROMPOSITION,
                static_cast<quintptr>(targetEnd)));
        for (int line = firstLine; line <= lastLine; ++line)
            SendScintilla(SCI_ENSUREVISIBLEENFORCEPOLICY, line);
    }
    SendScintillaNpp(
        SCI_SETSEL, static_cast<quintptr>(targetStart), targetEnd);
    _searchState.start = _searchState.forward ? targetEnd : targetStart;
    return true;
}

void ScintillaEditView::replace(const QString& replacement)
{
    if (!_searchState.active)
        return;

    const qintptr start = SendScintillaNpp(SCI_GETSELECTIONSTART);
    const qintptr originalLength =
        SendScintillaNpp(SCI_GETSELECTIONEND) - start;
    const QByteArray bytes = replacement.toUtf8();
    SendScintilla(SCI_TARGETFROMSELECTION);
    const qintptr replacementLength = SendScintillaNpp(
        (_searchState.flags & SCFIND_REGEXP)
            ? SCI_REPLACETARGETRE
            : SCI_REPLACETARGET,
        static_cast<quintptr>(bytes.size()),
        reinterpret_cast<qintptr>(bytes.constData()));
    SendScintillaNpp(
        SCI_SETSEL, static_cast<quintptr>(start),
        start + replacementLength);
    if (_searchState.forward) {
        _searchState.start = start + replacementLength;
        _searchState.end += replacementLength - originalLength;
    }
}

// ─── 语法高亮 ─────────────────────────────────────────────────────────────────

static QFont editorFont() { return QFont("Consolas", 10); }

// 扩展名 → Notepad++ 语言名（用于查 stylers.xml）
static QString extToNppName(const QString& ext, const QString& fileName)
{
    // 优先从 langs.xml 查找
    const LangDesc* ld = NppParameters::getInstance().getLangDescByExt(ext);
    if (ld) return ld->name;

    // 回退：硬编码常见映射
    if (ext=="c"||ext=="cpp"||ext=="cxx"||ext=="cc"||
        ext=="h"||ext=="hpp"||ext=="hxx"||ext=="inl") return "cpp";
    if (ext=="cs")     return "cs";
    if (ext=="java")   return "java";
    if (ext=="py"||ext=="pyw") return "python";
    if (ext=="js"||ext=="jsx"||ext=="ts"||ext=="tsx"||ext=="mjs") return "javascript";
    if (ext=="html"||ext=="htm"||ext=="php"||ext=="asp") return "html";
    if (ext=="css")    return "css";
    if (ext=="xml"||ext=="svg"||ext=="xsl"||ext=="xslt"||ext=="xsd") return "xml";
    if (ext=="sh"||ext=="bash"||ext=="zsh"||ext=="ksh") return "bash";
    if (ext=="bat"||ext=="cmd") return "batch";
    if (ext=="sql")    return "sql";
    if (ext=="lua")    return "lua";
    if (ext=="rb")     return "ruby";
    if (ext=="pl"||ext=="pm") return "perl";
    if (ext=="json")   return "json";
    if (ext=="yaml"||ext=="yml") return "yaml";
    if (ext=="md"||ext=="markdown") return "markdown";
    if (ext=="cmake"||fileName=="cmakelists.txt") return "cmake";
    if (fileName=="makefile"||fileName=="gnumakefile"||
        ext=="mak"||ext=="mk") return "makefile";
    if (ext=="tex"||ext=="sty") return "tex";
    if (ext=="diff"||ext=="patch") return "diff";
    if (ext=="vhd"||ext=="vhdl") return "vhdl";
    if (ext=="properties"||ext=="ini"||ext=="cfg") return "ini";
    return QString();
}

// Notepad++ 语言名到 Lexilla lexer 名称。
QString ScintillaEditView::builtinLexerName(const QString& nppLanguageName)
{
    const QString normalized = nppLanguageName.trimmed().toLower();
    for (const LexerNameMapping& mapping : lexerNameMappings) {
        if (normalized == QLatin1String(mapping.nppName))
            return QString::fromLatin1(mapping.scintillaName);
    }
    return QString();
}

bool ScintillaEditView::installLexer(const QString& lexerName,
                                     const LangDesc* language)
{
    Q_UNUSED(language);
    Scintilla::ILexer5* lexer = nullptr;
    if (!lexerName.isEmpty()) {
        const QByteArray name = lexerName.toLatin1();
        lexer = CreateLexer(name.constData());
        if (!lexer)
            return false;
    }

    send(SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(lexer));
    _lexerInstalled = lexer != nullptr;
    for (int set = 0; set < LangDesc::KeywordSetCount; ++set)
        setKeywordSlot(set, QString());
    return true;
}

void ScintillaEditView::setKeywordSlot(int slot, const QString& keywords)
{
    while (_installedKeywordSets.size() < LangDesc::KeywordSetCount)
        _installedKeywordSets.append(QString());
    if (slot >= 0 && slot < _installedKeywordSets.size())
        _installedKeywordSets[slot] = keywords;
    const QByteArray utf8 = keywords.toUtf8();
    send(SCI_SETKEYWORDS, slot,
         reinterpret_cast<sptr_t>(utf8.constData()));
}

void ScintillaEditView::setLexerProperty(const char* name, const char* value)
{
    send(SCI_SETPROPERTY, reinterpret_cast<uptr_t>(name),
         reinterpret_cast<sptr_t>(value));
}

QStringList ScintillaEditView::configuredKeywords(
    const QString& languageName) const
{
    QStringList result;
    result.reserve(LangDesc::KeywordSetCount);
    const NppParameters& parameters = NppParameters::getInstance();
    const LangDesc* language = parameters.getLangDescByName(languageName);
    for (int index = 0; index < LangDesc::KeywordSetCount; ++index)
        result.append(language ? language->keywords[index] : QString());

    const LexerStyler* styler = parameters.getLexerStyler(languageName);
    if (!styler)
        return result;
    for (const WordsStyle& style : styler->styles) {
        const int index = keywordClassIndex(style.keywordClass);
        const QString userWords = style.userKeywords.trimmed();
        if (index < 0 || index >= result.size() || userWords.isEmpty())
            continue;
        if (!result[index].isEmpty())
            result[index].append(QLatin1Char(' '));
        result[index].append(userWords);
    }
    return result;
}

void ScintillaEditView::configureLexer(const QString& languageName)
{
    const QString language = languageName.trimmed().toLower();
    for (int slot = 0; slot < LangDesc::KeywordSetCount; ++slot)
        setKeywordSlot(slot, QString());

    const QStringList words = configuredKeywords(language);
    auto sourceWords = [this](const QString& source, int index) {
        const QStringList sourceSets = configuredKeywords(source);
        return index >= 0 && index < sourceSets.size()
            ? sourceSets[index] : QString();
    };
    auto setFrom = [this, &sourceWords](int slot, const QString& source,
                                        int sourceIndex) {
        setKeywordSlot(slot, sourceWords(source, sourceIndex));
    };
    auto setCurrent = [this, &words](int slot, int sourceIndex) {
        if (sourceIndex >= 0 && sourceIndex < words.size())
            setKeywordSlot(slot, words[sourceIndex]);
    };

    const bool cppFamily = language == QStringLiteral("c") ||
        language == QStringLiteral("cpp") ||
        language == QStringLiteral("cs") ||
        language == QStringLiteral("java") ||
        language == QStringLiteral("rc") ||
        language == QStringLiteral("actionscript") ||
        language == QStringLiteral("swift");
    if (cppFamily) {
        setCurrent(0, 0);
        setCurrent(1, 2);
        if (language != QStringLiteral("rc"))
            setFrom(2, QStringLiteral("cpp"), 3);
        setLexerProperty("fold", "1");
        setLexerProperty("fold.compact", "0");
        setLexerProperty("fold.comment", "1");
        setLexerProperty("fold.cpp.comment.explicit", "0");
        setLexerProperty("fold.preprocessor", "1");
        setLexerProperty("lexer.cpp.track.preprocessor", "0");
    } else if (language == QStringLiteral("javascript")) {
        setCurrent(0, 0);
        setCurrent(1, 2);
        setFrom(2, QStringLiteral("cpp"), 3);
        setCurrent(3, 1);
        setLexerProperty("fold", "1");
        setLexerProperty("fold.compact", "0");
        setLexerProperty("fold.comment", "1");
        setLexerProperty("fold.cpp.comment.explicit", "0");
        setLexerProperty("fold.preprocessor", "1");
        setLexerProperty("lexer.cpp.track.preprocessor", "0");
        setLexerProperty("lexer.cpp.backquoted.strings", "1");
    } else if (language == QStringLiteral("typescript")) {
        setCurrent(0, 0);
        setCurrent(1, 2);
        setFrom(2, QStringLiteral("cpp"), 3);
        setLexerProperty("fold", "1");
        setLexerProperty("fold.compact", "0");
        setLexerProperty("fold.comment", "1");
        setLexerProperty("fold.cpp.comment.explicit", "0");
        setLexerProperty("fold.preprocessor", "1");
        setLexerProperty("lexer.cpp.track.preprocessor", "0");
        setLexerProperty("lexer.cpp.backquoted.strings", "1");
    } else if (language == QStringLiteral("objc")) {
        setCurrent(0, 0);
        setCurrent(1, 2);
        setFrom(2, QStringLiteral("cpp"), 3);
        setCurrent(3, 1);
        setCurrent(4, 3);
        setLexerProperty("fold", "1");
        setLexerProperty("fold.compact", "0");
        setLexerProperty("fold.comment", "1");
        setLexerProperty("fold.cpp.comment.explicit", "0");
        setLexerProperty("fold.preprocessor", "1");
    } else if (language == QStringLiteral("tcl")) {
        setCurrent(0, 0);
        setCurrent(1, 2);
    } else if (language == QStringLiteral("json")) {
        setCurrent(0, 0);
        setCurrent(1, 1);
        setLexerProperty("fold", "1");
        setLexerProperty("fold.compact", "0");
        setLexerProperty("fold.comment", "1");
        setLexerProperty("fold.preprocessor", "1");
    } else if (language == QStringLiteral("xml")) {
        setLexerProperty("lexer.xml.allow.scripts", "0");
        setLexerProperty("fold", "1");
        setLexerProperty("fold.compact", "0");
        setLexerProperty("fold.html", "1");
        setLexerProperty("fold.hypertext.comment", "1");
    } else if (language == QStringLiteral("html") ||
               language == QStringLiteral("php") ||
               language == QStringLiteral("asp") ||
               language == QStringLiteral("jsp")) {
        setFrom(0, QStringLiteral("html"), 0);
        setFrom(1, QStringLiteral("javascript.js"), 0);
        setFrom(2, QStringLiteral("vb"), 0);
        setFrom(4, QStringLiteral("php"), 0);
        setLexerProperty("asp.default.language", "2");
        setLexerProperty("fold", "1");
        setLexerProperty("fold.compact", "0");
        setLexerProperty("fold.html", "1");
        setLexerProperty("fold.hypertext.comment", "1");
        applyStylers(QStringLiteral("javascript.js"));
        applyStylers(QStringLiteral("php"));
        applyStylers(QStringLiteral("asp"));
        send(SCI_STYLESETEOLFILLED, SCE_HJ_DEFAULT, 1);
        send(SCI_STYLESETEOLFILLED, SCE_HJ_COMMENT, 1);
        send(SCI_STYLESETEOLFILLED, SCE_HJ_COMMENTDOC, 1);
        send(SCI_STYLESETEOLFILLED, SCE_HPHP_DEFAULT, 1);
        send(SCI_STYLESETEOLFILLED, SCE_HPHP_COMMENT, 1);
        send(SCI_STYLESETEOLFILLED, SCE_HBA_DEFAULT, 1);
    } else {
        quint16 mask = 0;
        bool found = false;
        for (const LanguageKeywordMask& entry : languageKeywordMasks) {
            if (language == QLatin1String(entry.name)) {
                mask = entry.mask;
                found = true;
                break;
            }
        }
        if (!found) {
            for (int index = 0; index < words.size(); ++index)
                if (!words[index].isEmpty())
                    mask |= static_cast<quint16>(1u << index);
        }
        for (int slot = 0; slot < words.size(); ++slot)
            if (mask & (1u << slot))
                setKeywordSlot(slot, words[slot]);
    }

    if (language == QStringLiteral("sql"))
        setLexerProperty("sql.backslash.escapes",
            NppParameters::getInstance().getNppGUI()
                ._backSlashIsEscapeCharacterForSql ? "1" : "0");
    if (language == QStringLiteral("pascal") ||
        language == QStringLiteral("autoit") ||
        language == QStringLiteral("verilog"))
        setLexerProperty("fold.preprocessor", "1");
    if (language == QStringLiteral("python"))
        setLexerProperty("fold.quotes.python", "1");
    if (language == QStringLiteral("ini"))
        send(SCI_STYLESETEOLFILLED, SCE_PROPS_SECTION, 1);
    if (language == QStringLiteral("ruby"))
        send(SCI_STYLESETEOLFILLED, SCE_RB_POD, 1);
    if (language == QStringLiteral("csound"))
        send(SCI_STYLESETEOLFILLED, SCE_CSOUND_STRINGEOL, 1);
    if (language == QStringLiteral("searchresult")) {
        send(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_FILE_HEADER, 1);
        send(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_SEARCH_HEADER, 1);
    }
    if (language == QStringLiteral("baanc")) {
        setLexerProperty("lexer.baan.styling.within.preprocessor", "1");
        setLexerProperty("fold.preprocessor", "1");
        setLexerProperty("fold.baan.syntax.based", "1");
        setLexerProperty("fold.baan.keywords.based", "1");
        setLexerProperty("fold.baan.sections", "1");
        setLexerProperty("fold.baan.inner.level", "1");
        send(SCI_SETWORDCHARS, 0, reinterpret_cast<sptr_t>(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$:"));
        send(SCI_STYLESETEOLFILLED, SCE_BAAN_STRINGEOL, 1);
    } else if (language == QStringLiteral("avs")) {
        send(SCI_SETWORDCHARS, 0, reinterpret_cast<sptr_t>(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#"));
    } else if (language == QStringLiteral("forth") ||
               language == QStringLiteral("nncrontab")) {
        send(SCI_SETWORDCHARS, 0, reinterpret_cast<sptr_t>(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789%-"));
    } else if (language == QStringLiteral("oscript")) {
        send(SCI_SETWORDCHARS, 0, reinterpret_cast<sptr_t>(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$"));
    } else if (language == QStringLiteral("rebol")) {
        send(SCI_SETWORDCHARS, 0, reinterpret_cast<sptr_t>(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789?!.'+-*&|=_~"));
    } else if (language == QStringLiteral("rust")) {
        send(SCI_SETWORDCHARS, 0, reinterpret_cast<sptr_t>(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#"));
    }
}

void ScintillaEditView::setLexerForFile(const QString& filePath)
{
    if (_largeFileMode) {
        clearLexer();
        return;
    }
    QFileInfo fi(filePath);
    QString ext      = fi.suffix().toLower();
    QString fileName = fi.fileName().toLower();

    const LangDesc* extensionLanguage =
        NppParameters::getInstance().getLangDescByExt(ext);
    QString nppName = extensionLanguage
        ? extensionLanguage->name : extToNppName(ext, fileName);
    _currentLexerName = nppName.toLower();

    if (nppName.isEmpty()) {
        const UserLangDesc* udl = NppParameters::getInstance().getUserLangByExt(ext);
        if (udl) {
            setUserDefinedLanguage(*udl);
            return;
        }
    }

    // 从 langs.xml 读取注释符号
    _commentLine.clear(); _commentStart.clear(); _commentEnd.clear();
    if (!nppName.isEmpty()) {
        const LangDesc* ld = NppParameters::getInstance().getLangDescByName(nppName);
        if (!ld) ld = NppParameters::getInstance().getLangDescByExt(ext);
        if (ld) {
            _commentLine  = ld->commentLine;
            _commentStart = ld->commentStart;
            _commentEnd   = ld->commentEnd;
        }
    }

    const QString lexerName = builtinLexerName(nppName);
    const LangDesc* language =
        NppParameters::getInstance().getLangDescByName(nppName);
    const bool installed = !lexerName.isEmpty() &&
        installLexer(lexerName, language);

    if (installed) {
        applyStylers(nppName);  // 从 stylers.xml 应用颜色
        configureLexer(nppName);
        setupAutoComplete();
    } else {
        installLexer(QString());
        setFont(editorFont());
    }
    applyGlobalStyles();
    scheduleUrlRefresh();
    refreshXmlTagHighlight();
}

void ScintillaEditView::setBuiltinLanguage(const QString& languageName)
{
    if (_largeFileMode)
        return;
    _currentLexerName = languageName.toLower();
    const QString lexerName = builtinLexerName(_currentLexerName);
    const LangDesc* language =
        NppParameters::getInstance().getLangDescByName(_currentLexerName);
    if (!lexerName.isEmpty() && installLexer(lexerName, language)) {
        applyStylers(_currentLexerName);
        configureLexer(_currentLexerName);
        setupAutoComplete();
    }
    applyGlobalStyles();
}

void ScintillaEditView::setUserDefinedLanguage(const UserLangDesc& language)
{
    if (_largeFileMode) {
        clearLexer();
        return;
    }
    _commentLine = language.lineComment;
    _currentLexerName = language.name.toLower();
    _commentStart = language.blockCommentStart;
    _commentEnd = language.blockCommentEnd;

    if (!installLexer(QStringLiteral("user")))
        return;
    UserDefinedLexer::configure(*this, language);
    setupAutoComplete();
    applyGlobalStyles();
}

// ─── stylers.xml 样式应用 ─────────────────────────────────────────────────────

void ScintillaEditView::applyStylers(const QString& nppLexerName)
{
    const LexerStyler* ls = NppParameters::getInstance().getLexerStyler(nppLexerName);
    if (!ls || !hasLexer()) return;

    NppGUI& gui = NppParameters::getInstance().getNppGUI();
    QFont   defaultFont(gui._editorFontName, gui._editorFontSize);

    for (const WordsStyle& ws : ls->styles) {
        QFont f = ws.fontName.isEmpty() ? defaultFont : QFont(ws.fontName, ws.fontSize > 0 ? ws.fontSize : gui._editorFontSize);
        if (ws.fontStyle & 1) f.setBold(true);
        if (ws.fontStyle & 2) f.setItalic(true);
        if (ws.fontStyle & 4) f.setUnderline(true);
        const QByteArray family = f.family().toUtf8();
        execute(SCI_STYLESETFONT, ws.styleID,
                reinterpret_cast<sptr_t>(family.constData()));
        execute(SCI_STYLESETSIZE, ws.styleID, f.pointSize());
        execute(SCI_STYLESETBOLD, ws.styleID, f.bold());
        execute(SCI_STYLESETITALIC, ws.styleID, f.italic());
        execute(SCI_STYLESETUNDERLINE, ws.styleID, f.underline());
        if (ws.hasFg) execute(SCI_STYLESETFORE, ws.styleID,
            ws.fgColor.red() | (ws.fgColor.green() << 8) | (ws.fgColor.blue() << 16));
        if (ws.hasBg) execute(SCI_STYLESETBACK, ws.styleID,
            ws.bgColor.red() | (ws.bgColor.green() << 8) | (ws.bgColor.blue() << 16));
    }

}

void ScintillaEditView::applyGlobalStyles()
{
    // 对应原版 ScintillaEditView::performGlobalStyles()
    // 按名称/styleID 查找并应用 GlobalStyles 配置

    const QVector<WordsStyle>& globals = NppParameters::getInstance().getGlobalStyles();
    if (globals.isEmpty()) return;

    // ── 辅助：按名称查找 ────────────────────────────────────────────────────────
    auto findByName = [&](const QString& n) -> const WordsStyle* {
        for (const auto& ws : globals)
            if (ws.name == n) return &ws;
        return nullptr;
    };
    // ── 辅助：QColor → Scintilla BGR long（Scintilla 颜色格式 0x00BBGGRR）────────
    auto toSci = [](const QColor& c) -> long {
        return c.red() | (c.green() << 8) | (c.blue() << 16);
    };

    const WordsStyle* pStyle;

    // ── Default Style (styleID=32)：编辑区默认前/背景 ─────────────────────────
    // 对应原版 setStyle(STYLE_DEFAULT, *pStyle)
    pStyle = findByName("Default Style");
    if (pStyle) {
        if (pStyle->hasBg) execute(SCI_STYLESETBACK, STYLE_DEFAULT, toSci(pStyle->bgColor));
        if (pStyle->hasFg) execute(SCI_STYLESETFORE, STYLE_DEFAULT, toSci(pStyle->fgColor));
    }

    // ── Line number margin (styleID=33) ──────────────────────────────────────
    pStyle = findByName("Line number margin");
    if (pStyle) {
        if (pStyle->hasFg) setMarginsForegroundColor(pStyle->fgColor);
        if (pStyle->hasBg) setMarginsBackgroundColor(pStyle->bgColor);
    }

    // ── Indent guideline style (styleID=37) ──────────────────────────────────
    pStyle = findByName("Indent guideline style");
    if (pStyle) {
        if (pStyle->hasFg) execute(SCI_STYLESETFORE, 37, toSci(pStyle->fgColor));
        if (pStyle->hasBg) execute(SCI_STYLESETBACK, 37, toSci(pStyle->bgColor));
    }

    // ── Brace highlight style (styleID=34，STYLE_BRACELIGHT) ─────────────────
    pStyle = findByName("Brace highlight style");
    if (pStyle) {
        if (pStyle->hasFg) execute(SCI_STYLESETFORE, 34, toSci(pStyle->fgColor));
        if (pStyle->hasBg) execute(SCI_STYLESETBACK, 34, toSci(pStyle->bgColor));
    }

    // ── Bad brace colour (styleID=35，STYLE_BRACEBAD) ────────────────────────
    pStyle = findByName("Bad brace colour");
    if (pStyle) {
        if (pStyle->hasFg) execute(SCI_STYLESETFORE, 35, toSci(pStyle->fgColor));
        if (pStyle->hasBg) execute(SCI_STYLESETBACK, 35, toSci(pStyle->bgColor));
    }

    // ── Current line background colour ───────────────────────────────────────
    // 对应原版 SCI_SETELEMENTCOLOUR SC_ELEMENT_CARET_LINE_BACK
    pStyle = findByName("Current line background colour");
    if (pStyle && pStyle->hasBg)
        setCaretLineBackgroundColor(pStyle->bgColor);

    // ── Selected text colour ─────────────────────────────────────────────────
    // 对应原版 SCI_SETSELBACK；原版 _isSelectFgColorEnabled 默认 false，不调用 SCI_SETSELFORE
    pStyle = findByName("Selected text colour");
    if (pStyle && pStyle->hasBg)
        setSelectionBackgroundColor(pStyle->bgColor);
    // 对应原版 _isSelectFgColorEnabled=false：不强制覆盖选区文字颜色，让文字保持原色。
    // 保持选区文字使用原样式前景色。
    // 设下的白色前景，确保选中文字不会变白。
    resetSelectionForegroundColor();  // SCI_SETSELFORE(0) → 文字保持原色

    // ── Caret colour (styleID=2069，SCI_SETCARETFORE) ───────────────────────
    pStyle = findByName("Caret colour");
    if (pStyle && pStyle->hasFg)
        setCaretForegroundColor(pStyle->fgColor);

    // ── Edge colour（列参考线颜色，SCI_SETEDGECOLOUR）────────────────────────
    pStyle = findByName("Edge colour");
    if (pStyle && pStyle->hasFg)
        execute(SCI_SETEDGECOLOUR, toSci(pStyle->fgColor));

    // ── Fold margin（折叠边距背景/高亮色）────────────────────────────────────
    // 对应原版 SCI_SETFOLDMARGINCOLOUR / SCI_SETFOLDMARGINHICOLOUR
    pStyle = findByName("Fold margin");
    if (pStyle) {
        if (pStyle->hasBg) execute(SCI_SETFOLDMARGINCOLOUR,   1, toSci(pStyle->bgColor));
        if (pStyle->hasFg) execute(SCI_SETFOLDMARGINHICOLOUR, 1, toSci(pStyle->fgColor));
    }

    // ── Bookmark margin（符号边距背景色）────────────────────────────────────
    // 原版：优先用 "Bookmark margin"，否则回退到 "Line number margin" 的 bgColor
    pStyle = findByName("Bookmark margin");
    if (!pStyle) pStyle = findByName("Line number margin");
    if (pStyle && pStyle->hasBg)
        execute(SCI_SETMARGINBACKN, _SC_MARGE_SYMBOL, toSci(pStyle->bgColor));

    // ── White space symbol（空白符颜色，SCI_SETWHITESPACEFORE）───────────────
    pStyle = findByName("White space symbol");
    if (pStyle && pStyle->hasFg)
        execute(SCI_SETWHITESPACEFORE, 1, toSci(pStyle->fgColor));

    // ── 指示器颜色（Smart Highlighting、Find Mark 等，bgColor → SCI_INDICSETFORE）
    // 对应原版 setStyle() 对 indicator-range styleID 的处理
    static const struct { const char* name; int id; } kIndicStyles[] = {
        { "Smart Highlighting",          29 },
        { "Find Mark Style",             31 },
        { "Incremental highlight all",   28 },
        { "Tags match highlighting",     27 },
        { "Tags attribute",              26 },
        { "Mark Style 1",                25 },
        { "Mark Style 2",                24 },
        { "Mark Style 3",                23 },
        { "Mark Style 4",                22 },
        { "Mark Style 5",                21 },
    };
    for (const auto& is : kIndicStyles) {
        pStyle = findByName(is.name);
        if (!pStyle) continue;
        // bgColor → 指示器填充色（SCI_INDICSETFORE）
        if (pStyle->hasBg)
            execute(SCI_INDICSETFORE, is.id, toSci(pStyle->bgColor));
        // fgColor → 文字前景色（对应原版 SCI_STYLESETFORE，用于 lexer style 29 等）
        if (pStyle->hasFg)
            execute(SCI_STYLESETFORE, is.id, toSci(pStyle->fgColor));
    }
}

void ScintillaEditView::reloadConfiguredStyles()
{
    if (hasLexer() && !_currentLexerName.isEmpty()) {
        applyStylers(_currentLexerName);
        configureLexer(_currentLexerName);
    }
    applyGlobalStyles();
}

void ScintillaEditView::setLexerByExtension(const QString& ext)
{
    // 构造一个临时路径来复用 setLexerForFile 的扩展名逻辑
    QString name = (ext == "makefile") ? "Makefile" : ("_dummy." + ext);
    setLexerForFile(name);
}

bool ScintillaEditView::setLexerByName(const QString& name)
{
    if (_largeFileMode)
        return false;
    const QString normalized = name.trimmed().toLower();
    if (normalized.isEmpty() || normalized == QStringLiteral("text") ||
        normalized == QStringLiteral("normal") ||
        normalized == QStringLiteral("normal text")) {
        clearLexer();
        return true;
    }

    const QString lexerName = builtinLexerName(normalized);
    if (lexerName.isEmpty())
        return false;
    _currentLexerName = normalized;
    _commentLine.clear();
    _commentStart.clear();
    _commentEnd.clear();
    if (const LangDesc* language =
            NppParameters::getInstance().getLangDescByName(normalized)) {
        _commentLine = language->commentLine;
        _commentStart = language->commentStart;
        _commentEnd = language->commentEnd;
    }
    if (!installLexer(lexerName,
            NppParameters::getInstance().getLangDescByName(normalized)))
        return false;
    applyStylers(normalized);
    configureLexer(normalized);
    setupAutoComplete();
    applyGlobalStyles();
    scheduleUrlRefresh();
    refreshXmlTagHighlight();
    return true;
}

bool ScintillaEditView::installExternalLexer(
    const QString& name, ExternalLexerFactory factory)
{
    if (_largeFileMode || !factory || name.trimmed().isEmpty())
        return false;
    const QByteArray lexerName = name.toLatin1();
    Scintilla::ILexer5* lexer = factory(lexerName.constData());
    if (!lexer)
        return false;

    send(SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(lexer));
    _lexerInstalled = true;
    _currentLexerName = name.trimmed().toLower();
    for (int slot = 0; slot < LangDesc::KeywordSetCount; ++slot)
        setKeywordSlot(slot, QString());
    applyStylers(_currentLexerName);
    configureLexer(_currentLexerName);
    setupAutoComplete();
    applyGlobalStyles();
    return true;
}

void ScintillaEditView::clearLexer()
{
    _currentLexerName.clear();
    installLexer(QString());
    _completionWords.clear();
    _completionCallTips.clear();
    setFont(editorFont());
    applyGlobalStyles();
}

// ─── 行注释切换 ───────────────────────────────────────────────────────────────

void ScintillaEditView::toggleLineComment()
{
    // 优先使用从 langs.xml 读取的行注释符
    QString commentStr = _commentLine;

    // 回退：从当前 Notepad++ 语言名推断。
    if (commentStr.isEmpty()) {
        const QString lang = _currentLexerName;
        if (lang.isEmpty()) return;
        if (lang.contains("c++") || lang.contains("c#") || lang.contains("java") ||
            lang == "javascript" || lang == "css") {
            commentStr = "//";
        } else if (lang == "python" || lang == "bash" || lang == "ruby" ||
                   lang == "perl"   || lang == "cmake"  || lang == "yaml" ||
                   lang == "makefile") {
            commentStr = "#";
        } else if (lang == "sql" || lang == "lua" || lang == "vhdl") {
            commentStr = "--";
        } else if (lang == "tex") {
            commentStr = "%";
        }
    }
    if (commentStr.isEmpty()) return;

    // 确定操作行范围
    int fromLine, fromIdx, toLine, toIdx;
    if (hasSelectedText()) {
        getSelection(&fromLine, &fromIdx, &toLine, &toIdx);
        // 选区刚好到行首时不包含最后一行
        if (toIdx == 0 && toLine > fromLine) --toLine;
    } else {
        getCursorPosition(&fromLine, &fromIdx);
        toLine = fromLine;
    }

    // 判断所有非空行是否都已注释
    bool allCommented = true;
    for (int line = fromLine; line <= toLine; ++line) {
        QString t = text(line).trimmed();
        if (!t.isEmpty() && !t.startsWith(commentStr)) {
            allCommented = false;
            break;
        }
    }

    beginUndoAction();
    for (int line = fromLine; line <= toLine; ++line) {
        QString lineText = text(line);
        if (allCommented) {
            int pos = lineText.indexOf(commentStr);
            if (pos >= 0) {
                setSelection(line, pos, line, pos + commentStr.length());
                removeSelectedText();
            }
        } else {
            if (!lineText.trimmed().isEmpty())
                insertAt(commentStr, line, 0);
        }
    }
    endUndoAction();
}

void ScintillaEditView::blockComment()
{
    if (!hasSelectedText()) return;
    QString start = _commentStart;
    QString end = _commentEnd;
    if (start.isEmpty() || end.isEmpty()) {
        start = "/*";
        end = "*/";
    }
    const QString selection = selectedText();
    beginUndoAction();
    replaceSelectedText(start + selection + end);
    endUndoAction();
}

void ScintillaEditView::blockUncomment()
{
    if (!hasSelectedText()) return;
    QString start = _commentStart;
    QString end = _commentEnd;
    if (start.isEmpty() || end.isEmpty()) {
        start = "/*";
        end = "*/";
    }
    QString selection = selectedText();
    if (!selection.startsWith(start) || !selection.endsWith(end)) return;
    selection.remove(0, start.size());
    selection.chop(end.size());
    beginUndoAction();
    replaceSelectedText(selection);
    endUndoAction();
}

// ─── 自动完成 ─────────────────────────────────────────────────────────────────

void ScintillaEditView::setupAutoComplete()
{
    if (!hasLexer()) return;

    QStringList words;
    if (const LangDesc* language = NppParameters::getInstance()
            .getLangDescByName(_currentLexerName)) {
        for (int set = 0; set < LangDesc::KeywordSetCount; ++set)
            words.append(language->keywords[set].split(
                QLatin1Char(' '), QString::SkipEmptyParts));
    }
    const NppParameters& parameters = NppParameters::getInstance();
    const QVector<AutoCompletionEntry> configured =
        AutoCompletionParser::loadLanguage(
            _currentLexerName, parameters.getUserPath(),
            parameters.getNppPath());
    QStringList callTips;
    for (const AutoCompletionEntry& entry : configured) {
        const QString apiText = entry.apiText();
        words.append(apiText);
        if (!entry.parameters.isEmpty())
            callTips.append(apiText);
    }
    words.removeDuplicates();
    words.sort(Qt::CaseInsensitive);
    _completionWords = words;
    _completionCallTips = callTips;

    // 从文档单词和 API 两处补全
    setAutoCompletionSource(AcsAll);
    setAutoCompletionThreshold(3);
    setAutoCompletionCaseSensitivity(false);
    setAutoCompletionReplaceWord(true);
}

void ScintillaEditView::applyAutoComplete(bool enable, int threshold)
{
    if (_largeFileMode)
        enable = false;
    if (enable) {
        setAutoCompletionSource(AcsAll);
        setAutoCompletionThreshold(threshold);
    } else {
        setAutoCompletionSource(AcsNone);
    }
}

// ─── 书签 ─────────────────────────────────────────────────────────────────────

void ScintillaEditView::onMarginClicked(int margin, int line, Qt::KeyboardModifiers)
{
    if (margin == _SC_MARGE_SYMBOL)
        toggleBookmark(line);
}

void ScintillaEditView::toggleBookmark()
{
    int line, index;
    getCursorPosition(&line, &index);
    toggleBookmark(line);
}

void ScintillaEditView::toggleBookmark(int line)
{
    if (line < 0 || line >= lines())
        return;
    if (markersAtLine(line) & (1 << BOOKMARK_MARKER))
        markerDelete(line, BOOKMARK_MARKER);
    else
        markerAdd(line, BOOKMARK_MARKER);
}

void ScintillaEditView::nextBookmark()
{
    int line, index;
    getCursorPosition(&line, &index);
    int found = markerFindNext(line + 1, 1 << BOOKMARK_MARKER);
    if (found == -1)  // 到文件末尾未找到，从头循环
        found = markerFindNext(0, 1 << BOOKMARK_MARKER);
    if (found != -1) {
        setCursorPosition(found, 0);
        ensureLineVisible(found);
    }
}

void ScintillaEditView::prevBookmark()
{
    int line, index;
    getCursorPosition(&line, &index);
    int found = markerFindPrevious(line - 1, 1 << BOOKMARK_MARKER);
    if (found == -1)  // 到文件开头未找到，从尾循环
        found = markerFindPrevious(lines() - 1, 1 << BOOKMARK_MARKER);
    if (found != -1) {
        setCursorPosition(found, 0);
        ensureLineVisible(found);
    }
}

void ScintillaEditView::clearAllBookmarks()
{
    markerDeleteAll(BOOKMARK_MARKER);
}

// ─── 偏好设置应用 ─────────────────────────────────────────────────────────────

void ScintillaEditView::applyFont(const QString& family, int size)
{
    QFont f(family, size);
    setFont(f);
    // 同步更新词法分析器字体
    reloadConfiguredStyles();
}

void ScintillaEditView::applyTabSettings(int width, bool useSpaces)
{
    setTabWidth(width);
    setIndentationsUseTabs(!useSpaces);
}

void ScintillaEditView::applyWordWrap(bool enable)
{
    if (_largeFileMode)
        enable = false;
    setWrapMode(enable ? WrapWord : WrapNone);
}

void ScintillaEditView::applyShowWhitespace(bool show)
{
    setWhitespaceVisibility(show ? WsVisible : WsInvisible);
}

void ScintillaEditView::applyShowEol(bool show)
{
    setEolVisibility(show);
}

void ScintillaEditView::applyShowIndentGuide(bool show)
{
    // 对应原版 showIndentGuideLine()：
    // 使用 SC_IV_LOOKBOTH，与原版非 Python 语言默认行为一致。
    // SC_IV_LOOKBOTH 会同时向前后查找非空行确定缩进级别，显示更完整的缩进线
    execute(SCI_SETINDENTATIONGUIDES, show ? SC_IV_LOOKBOTH : SC_IV_NONE);
}

// ─── Smart Highlighting ───────────────────────────────────────────────────────

void ScintillaEditView::scheduleUrlRefresh()
{
    if (_urlRefreshTimer)
        _urlRefreshTimer->start();
}

static qintptr utf8Offset(const QString& text, int utf16Offset)
{
    return text.left(utf16Offset).toUtf8().size();
}

void ScintillaEditView::refreshUrlHotspots()
{
    const NppGUI& gui = NppParameters::getInstance().getNppGUI();
    const qintptr length = documentLengthNpp();
    SendScintillaNpp(SCI_SETINDICATORCURRENT, URL_INDICATOR);
    SendScintillaNpp(SCI_INDICATORCLEARRANGE, 0, length);
    _urlRanges.clear();
    if (_largeFileMode || gui._urlMode == 0 || length == 0)
        return;

    const bool underline = gui._urlMode == 2 || gui._urlMode == 4;
    execute(SCI_INDICSETSTYLE, URL_INDICATOR,
            underline ? INDIC_PLAIN : INDIC_HIDDEN);
    execute(SCI_INDICSETHOVERSTYLE, URL_INDICATOR,
            (gui._urlMode == 3 || gui._urlMode == 4)
                ? INDIC_FULLBOX : INDIC_PLAIN);
    QStringList schemes = {
        QStringLiteral("http://"), QStringLiteral("https://"),
        QStringLiteral("ftp://"), QStringLiteral("file://"),
        QStringLiteral("mailto:")
    };
    schemes.append(gui._uriCustomizedSchemes.split(
        QRegularExpression(QStringLiteral("\\s+")), QString::SkipEmptyParts));
    QStringList escaped;
    for (const QString& scheme : schemes)
        escaped.append(QRegularExpression::escape(scheme));
    const QRegularExpression expression(
        QStringLiteral("(?i)(?:%1)[^\\s<>\"']+").arg(escaped.join('|')));
    const QString source = text();
    QRegularExpressionMatchIterator matches = expression.globalMatch(source);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        QString value = match.captured();
        while (!value.isEmpty() &&
               QStringLiteral(".,;:!?)]}").contains(value.back()))
            value.chop(1);
        if (value.isEmpty())
            continue;
        const qintptr start = utf8Offset(source, match.capturedStart());
        const qintptr end = start + value.toUtf8().size();
        SendScintillaNpp(SCI_INDICATORFILLRANGE, start, end - start);
        _urlRanges.append(qMakePair(start, end));
    }
}

void ScintillaEditView::refreshXmlTagHighlight()
{
    const NppGUI& gui = NppParameters::getInstance().getNppGUI();
    const qintptr length = documentLengthNpp();
    SendScintillaNpp(SCI_SETINDICATORCURRENT, XML_TAG_INDICATOR);
    SendScintillaNpp(SCI_INDICATORCLEARRANGE, 0, length);
    const bool xmlLanguage = _currentLexerName == QStringLiteral("xml") ||
        _currentLexerName == QStringLiteral("html") ||
        _currentLexerName == QStringLiteral("php") ||
        _currentLexerName == QStringLiteral("asp") ||
        _currentLexerName == QStringLiteral("jsp");
    if (_largeFileMode || !gui._enableTagsMatchHighlight || !xmlLanguage)
        return;

    struct Tag {
        QString name;
        int start = 0;
        int end = 0;
        bool closing = false;
        bool selfClosing = false;
    };
    QVector<Tag> tags;
    const QString source = text();
    const QRegularExpression expression(
        QStringLiteral("<\\s*(/?)\\s*([A-Za-z_][\\w:.-]*)\\b[^<>]*(/?)\\s*>"));
    QRegularExpressionMatchIterator iterator = expression.globalMatch(source);
    while (iterator.hasNext()) {
        const QRegularExpressionMatch match = iterator.next();
        tags.append({match.captured(2), match.capturedStart(),
                     match.capturedEnd(), !match.captured(1).isEmpty(),
                     !match.captured(3).isEmpty()});
    }
    const qintptr caretBytes = currentPositionNpp();
    const int caret = QString::fromUtf8(source.toUtf8().left(caretBytes)).size();
    int current = -1;
    for (int i = 0; i < tags.size(); ++i) {
        if (caret >= tags[i].start && caret <= tags[i].end) {
            current = i;
            break;
        }
    }
    if (current < 0 || tags[current].selfClosing)
        return;

    const Qt::CaseSensitivity sensitivity =
        _currentLexerName == QStringLiteral("html")
            ? Qt::CaseInsensitive : Qt::CaseSensitive;
    int matchIndex = -1;
    int depth = 0;
    if (!tags[current].closing) {
        for (int i = current + 1; i < tags.size(); ++i) {
            if (tags[i].name.compare(tags[current].name, sensitivity) != 0 ||
                tags[i].selfClosing)
                continue;
            if (!tags[i].closing)
                ++depth;
            else if (depth-- == 0) {
                matchIndex = i;
                break;
            }
        }
    } else {
        for (int i = current - 1; i >= 0; --i) {
            if (tags[i].name.compare(tags[current].name, sensitivity) != 0 ||
                tags[i].selfClosing)
                continue;
            if (tags[i].closing)
                ++depth;
            else if (depth-- == 0) {
                matchIndex = i;
                break;
            }
        }
    }
    if (matchIndex < 0)
        return;
    for (int index : {current, matchIndex}) {
        const qintptr start = utf8Offset(source, tags[index].start);
        const qintptr end = utf8Offset(source, tags[index].end);
        SendScintillaNpp(SCI_INDICATORFILLRANGE, start, end - start);
    }
}

void ScintillaEditView::onCharacterAdded(int character)
{
    if (_largeFileMode || _autoPairEditing)
        return;
    const NppGUI& gui = NppParameters::getInstance().getNppGUI();
    const qintptr caret = currentPositionNpp();
    const char next = static_cast<char>(
        SendScintillaNpp(SCI_GETCHARAT, static_cast<quintptr>(caret)));
    char closer = '\0';
    if (character == '(' && gui._autoInsertParentheses) closer = ')';
    if (character == '[' && gui._autoInsertBrackets) closer = ']';
    if (character == '{' && gui._autoInsertCurlyBrackets) closer = '}';
    if (character == '\'' && gui._autoInsertQuotes) closer = '\'';
    if (character == '"' && gui._autoInsertDoubleQuotes) closer = '"';
    if ((character == ')' && gui._autoInsertParentheses) ||
        (character == ']' && gui._autoInsertBrackets) ||
        (character == '}' && gui._autoInsertCurlyBrackets)) {
        if (next == character) {
            _autoPairEditing = true;
            SendScintillaNpp(SCI_DELETERANGE, caret, 1);
            _autoPairEditing = false;
        }
        return;
    }
    if (closer != '\0') {
        const bool allowed = next == '\0' || next == ' ' || next == '\t' ||
            next == '\r' || next == '\n' || next == ')' || next == ']' ||
            next == '}';
        if (!allowed)
            return;
        const QByteArray inserted(1, closer);
        _autoPairEditing = true;
        SendScintillaNpp(SCI_INSERTTEXT, caret,
                         reinterpret_cast<qintptr>(inserted.constData()));
        _autoPairEditing = false;
        return;
    }
    if (character == '>' && gui._autoInsertHtmlXmlTag &&
        (_currentLexerName == QStringLiteral("xml") ||
         _currentLexerName == QStringLiteral("html"))) {
        const QString before =
            QString::fromUtf8(text().toUtf8().left(currentPositionNpp()));
        const QRegularExpression openTag(
            QStringLiteral("<([A-Za-z_][\\w:.-]*)(?:\\s[^<>]*)?>$"));
        const QRegularExpressionMatch match = openTag.match(before);
        if (match.hasMatch() && !before.endsWith(QStringLiteral("/>"))) {
            const QByteArray closeTag =
                QStringLiteral("</%1>").arg(match.captured(1)).toUtf8();
            _autoPairEditing = true;
            SendScintillaNpp(SCI_INSERTTEXT, caret,
                             reinterpret_cast<qintptr>(closeTag.constData()));
            _autoPairEditing = false;
        }
    }

    if (_autoCompletionSource != AcsNone && _autoCompletionThreshold > 0) {
        const qintptr current = send(SCI_GETCURRENTPOS);
        const qintptr start = send(SCI_WORDSTARTPOSITION, current, 1);
        if (current - start >= _autoCompletionThreshold)
            autoCompleteFromAPIs();
    }
}

void ScintillaEditView::onIndicatorClicked(int position, int)
{
    for (const auto& range : _urlRanges) {
        if (position < range.first || position > range.second)
            continue;
        const QByteArray utf8 = text().toUtf8().mid(
            static_cast<int>(range.first),
            static_cast<int>(range.second - range.first));
        QDesktopServices::openUrl(QUrl(QString::fromUtf8(utf8)));
        return;
    }
}

QString ScintillaEditView::markedText(int indicator) const
{
    const qintptr length = documentLengthNpp();
    QList<QByteArray> ranges;
    bool containsLineEnding = false;
    qintptr position = 0;
    while (position < length) {
        const qintptr rangeEnd = SendScintillaNpp(
            SCI_INDICATOREND,
            static_cast<quintptr>(indicator), position);
        if (rangeEnd <= position)
            break;
        if (SendScintillaNpp(SCI_INDICATORVALUEAT,
                             static_cast<quintptr>(indicator), position) != 0) {
            const char* data = utf8RangePointer(position, rangeEnd - position);
            if (data) {
                const QByteArray range(
                    data, static_cast<int>(rangeEnd - position));
                containsLineEnding = containsLineEnding
                    || range.contains('\r') || range.contains('\n');
                ranges.append(range);
            }
        }
        position = rangeEnd;
    }
    if (ranges.isEmpty())
        return QString();

    const QByteArray delimiter =
        containsLineEnding && ranges.size() > 1
            ? QByteArray("\r\n----\r\n")
            : QByteArray("\r\n");
    QByteArray joined = ranges.first();
    for (int i = 1; i < ranges.size(); ++i)
        joined += delimiter + ranges.at(i);
    if (ranges.size() > 1)
        joined += "\r\n";
    return QString::fromUtf8(joined);
}

void ScintillaEditView::updateSmartHighlight(int updated)
{
    static const int MAXLINEHIGHLIGHT = 400;

    if (_largeFileMode)
        return;

    // 先清除整个文档的 Smart Highlight 指示器
    int docLen = (int)SendScintilla(SCI_GETLENGTH);
    SendScintilla(SCI_SETINDICATORCURRENT, SMART_HIGHLIGHT_INDICATOR);
    SendScintilla(SCI_INDICATORCLEARRANGE, 0, docLen);
    const NppGUI& gui = NppParameters::getInstance().getNppGUI();
    if (!gui._smartHighlight)
        return;

    // 用底层 Scintilla 消息判断选区
    long selStart = (long)SendScintilla(SCI_GETSELECTIONSTART);
    long selEnd   = (long)SendScintilla(SCI_GETSELECTIONEND);
    if (selStart >= selEnd)
        return;

    int selLen = (int)(selEnd - selStart);
    if (selLen <= 0 || selLen > 1000)
        return;

    // 直接读取选区文本
    QByteArray bytes(selLen + 1, '\0');
    SendScintilla(SCI_GETSELTEXT, 0,
                  reinterpret_cast<sptr_t>(bytes.data()));
    bytes.resize(selLen);

    // 多行选区 → 不高亮
    if (bytes.contains('\n') || bytes.contains('\r'))
        return;

    // 用像素坐标反查可视行范围，避免 SCI_GETFIRSTVISIBLELINE/SCI_LINESONSCREEN 在
    // 使用完整可见范围，避免选区位于屏幕上半部分时落在搜索范围外。
    int viewH = viewport()->height();
    long firstPos = (long)SendScintilla(SCI_POSITIONFROMPOINT, (unsigned long)0, (long)0);
    long lastPos  = (long)SendScintilla(SCI_POSITIONFROMPOINT, (unsigned long)0,
                                        (long)(viewH > 1 ? viewH - 1 : 0));
    int firstLine = (int)SendScintilla(SCI_LINEFROMPOSITION, (unsigned long)firstPos);
    int lastLine  = (int)SendScintilla(SCI_LINEFROMPOSITION, (unsigned long)lastPos);
    int totalLines = (int)SendScintilla(SCI_GETLINECOUNT);
    if (lastLine >= totalLines) lastLine = totalLines - 1;

    // 按整词搜索（SCFIND_WHOLEWORD = 2）
    unsigned long searchFlags = 0;
    if (gui._smartHighlightWholeWord)
        searchFlags |= SCFIND_WHOLEWORD;
    if (gui._smartHighlightMatchCase)
        searchFlags |= SCFIND_MATCHCASE;
    SendScintilla(SCI_SETSEARCHFLAGS, searchFlags);
    SendScintilla(SCI_SETINDICATORCURRENT, SMART_HIGHLIGHT_INDICATOR);

    for (int line = firstLine; line <= lastLine; ++line) {
        long lineStart = (long)SendScintilla(SCI_POSITIONFROMLINE, (unsigned long)line);
        long lineEnd   = (long)SendScintilla(SCI_GETLINEENDPOSITION, (unsigned long)line);
        if (lineStart < 0 || lineEnd <= lineStart)
            continue;

        SendScintilla(SCI_SETTARGETSTART, (unsigned long)lineStart);
        SendScintilla(SCI_SETTARGETEND,   (unsigned long)lineEnd);

        while (true) {
            long found = (long)SendScintilla(SCI_SEARCHINTARGET,
                                             (uintptr_t)selLen,
                                             reinterpret_cast<sptr_t>(bytes.constData()));
            if (found < 0)
                break;

            long matchEnd = (long)SendScintilla(SCI_GETTARGETEND);
            long matchLen = matchEnd - found;
            if (matchLen <= 0)
                break;

            SendScintilla(SCI_INDICATORFILLRANGE, (unsigned long)found, (long)matchLen);

            // 推进搜索起点，避免死循环
            SendScintilla(SCI_SETTARGETSTART, (unsigned long)matchEnd);
            SendScintilla(SCI_SETTARGETEND,   (unsigned long)lineEnd);
        }
    }
}
