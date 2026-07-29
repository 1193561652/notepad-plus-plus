// ScintillaEditView.cpp - Scintilla 编辑器视图实现
// 移植自: v8.4.6:PowerEditor/src/ScintillaComponent/ScintillaEditView.cpp

#include "ScintillaEditView.h"
#include "../../third_party/boostregex/BoostRegexSearch.h"
#include "UserDefinedLexer.h"
#include "../Parameters.h"
#include <Qsci/qsciapis.h>
#include <Qsci/qscidocument.h>
#include <Qsci/qscilexerbash.h>
#include <Qsci/qscilexerbatch.h>
#include <Qsci/qscilexercmake.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexercsharp.h>
#include <Qsci/qscilexercss.h>
#include <Qsci/qscilexerdiff.h>
#include <Qsci/qscilexerhtml.h>
#include <Qsci/qscilexerjava.h>
#include <Qsci/qscilexerjavascript.h>
#include <Qsci/qscilexerjson.h>
#include <Qsci/qscilexerlua.h>
#include <Qsci/qscilexermakefile.h>
#include <Qsci/qscilexermarkdown.h>
#include <Qsci/qscilexerperl.h>
#include <Qsci/qscilexerproperties.h>
#include <Qsci/qscilexerpython.h>
#include <Qsci/qscilexerruby.h>
#include <Qsci/qscilexersql.h>
#include <Qsci/qscilexertex.h>
#include <Qsci/qscilexervhdl.h>
#include <Qsci/qscilexerxml.h>
#include <Qsci/qscilexeryaml.h>
#include <QFileInfo>
#include <QFont>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>

// 静态成员初始化
const int ScintillaEditView::_SC_MARGE_LINENUMBER;
const int ScintillaEditView::_SC_MARGE_SYMBOL;
const int ScintillaEditView::_SC_MARGE_FOLDER;
const int ScintillaEditView::URL_INDICATOR;
const int ScintillaEditView::XML_TAG_INDICATOR;

ScintillaEditView::ScintillaEditView(QWidget *parent)
    : QsciScintilla(parent)
{
    // 初始化编辑器
    init();
}

ScintillaEditView::~ScintillaEditView()
{
    // Qt 会自动清理
}

bool ScintillaEditView::createLargeDocument()
{
    QsciDocument largeDocument(
        SC_DOCUMENTOPTION_STYLES_NONE | SC_DOCUMENTOPTION_TEXT_LARGE);
    setDocument(largeDocument);
    const qintptr options = SendScintillaNpp(SCI_GETDOCUMENTOPTIONS);
    return (options & SC_DOCUMENTOPTION_STYLES_NONE) &&
           (options & SC_DOCUMENTOPTION_TEXT_LARGE);
}

void ScintillaEditView::createStandardDocument()
{
    QsciDocument standardDocument;
    setDocument(standardDocument);
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
        setBraceMatching(QsciScintilla::NoBraceMatch);
        QsciScintilla::setWrapMode(QsciScintilla::WrapNone);
        QsciScintilla::setAutoCompletionSource(QsciScintilla::AcsNone);
        QsciScintilla::setAutoCompletionThreshold(-1);
    } else {
        setBraceMatching(QsciScintilla::SloppyBraceMatch);
    }
}

void ScintillaEditView::init()
{
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
    setUtf8(true);  // 使用 UTF-8 编码
    setEolMode(QsciScintilla::EolWindows);  // Windows 行尾

    // 设置制表符
    setTabWidth(4);
    setIndentationsUseTabs(true);

    // 设置自动缩进
    setAutoIndent(true);

    // 设置括号匹配
    setBraceMatching(QsciScintilla::SloppyBraceMatch);

    // 边距点击（切换书签）
    connect(this, SIGNAL(marginClicked(int,int,Qt::KeyboardModifiers)),
            this, SLOT(onMarginClicked(int,int,Qt::KeyboardModifiers)));

    // Smart Highlighting：通过 SCN_UPDATEUI 驱动（选区变化/滚动均触发）
    connect(this, SIGNAL(SCN_UPDATEUI(int)),
            this, SLOT(updateSmartHighlight(int)));
    connect(this, SIGNAL(SCN_CHARADDED(int)),
            this, SLOT(onCharacterAdded(int)));
    connect(this, SIGNAL(SCN_INDICATORRELEASE(int,int)),
            this, SLOT(onIndicatorClicked(int,int)));
    connect(this, &QsciScintilla::textChanged,
            this, &ScintillaEditView::scheduleUrlRefresh);
    connect(this, &QsciScintilla::cursorPositionChanged,
            this, [this](int, int) { refreshXmlTagHighlight(); });

    // 当前行背景高亮（对应原版 SCI_SETCARETLINEVISIBLEALWAYS）
    // 颜色由 applyGlobalStyles() 从 stylers.xml "Current line background colour" 读取
    setCaretLineVisible(true);
    execute(SCI_SETCARETLINEVISIBLEALWAYS, 1);  // 失焦时仍保持高亮

    // 默认无语法高亮（新建文件为纯文本）

    // 应用全局样式（选区颜色等），stylers.xml 由 NppParameters::load() 预先加载
    applyGlobalStyles();
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
    setMarginType(_SC_MARGE_LINENUMBER, QsciScintilla::NumberMargin);
    setMarginWidth(_SC_MARGE_LINENUMBER, "00000");
    setMarginsForegroundColor(QColor(128, 128, 128));
    setMarginsBackgroundColor(QColor(240, 240, 240));

    // 符号边距（书签等）
    setMarginType(_SC_MARGE_SYMBOL, QsciScintilla::SymbolMargin);
    setMarginWidth(_SC_MARGE_SYMBOL, 16);
    setMarginSensitivity(_SC_MARGE_SYMBOL, true);

    // 书签标记：蓝色圆形
    markerDefine(QsciScintilla::Circle, BOOKMARK_MARKER);
    setMarkerBackgroundColor(QColor(50, 130, 255), BOOKMARK_MARKER);
    setMarkerForegroundColor(QColor(255, 255, 255), BOOKMARK_MARKER);

    // 折叠边距
    setMarginType(_SC_MARGE_FOLDER, QsciScintilla::SymbolMargin);
    setMarginWidth(_SC_MARGE_FOLDER, 14);
    setFolding(QsciScintilla::BoxedTreeFoldStyle);
    setMarginSensitivity(_SC_MARGE_FOLDER, true);
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
    QsciScintilla::setText(text);
}

QString ScintillaEditView::getText() const
{
    return text();
}

void ScintillaEditView::appendText(const QString &text)
{
    append(text);
}

void ScintillaEditView::insertText(int pos, const QString &text)
{
    insertAt(text, 0, pos);  // QScintilla 使用 (line, index)
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

// Notepad++ 语言名 → QsciLexer 实例
static QsciLexer* createLexer(const QString& nppName, QObject* parent)
{
    if (nppName=="cpp"||nppName=="c"||nppName=="objc") return new QsciLexerCPP(parent);
    if (nppName=="cs")         return new QsciLexerCSharp(parent);
    if (nppName=="java")       return new QsciLexerJava(parent);
    if (nppName=="python")     return new QsciLexerPython(parent);
    if (nppName=="javascript") return new QsciLexerJavaScript(parent);
    if (nppName=="html"||nppName=="php") return new QsciLexerHTML(parent);
    if (nppName=="css")        return new QsciLexerCSS(parent);
    if (nppName=="xml")        return new QsciLexerXML(parent);
    if (nppName=="bash")       return new QsciLexerBash(parent);
    if (nppName=="batch")      return new QsciLexerBatch(parent);
    if (nppName=="sql")        return new QsciLexerSQL(parent);
    if (nppName=="lua")        return new QsciLexerLua(parent);
    if (nppName=="ruby")       return new QsciLexerRuby(parent);
    if (nppName=="perl")       return new QsciLexerPerl(parent);
    if (nppName=="json")       return new QsciLexerJSON(parent);
    if (nppName=="yaml")       return new QsciLexerYAML(parent);
    if (nppName=="markdown")   return new QsciLexerMarkdown(parent);
    if (nppName=="cmake")      return new QsciLexerCMake(parent);
    if (nppName=="makefile")   return new QsciLexerMakefile(parent);
    if (nppName=="tex")        return new QsciLexerTeX(parent);
    if (nppName=="diff")       return new QsciLexerDiff(parent);
    if (nppName=="vhdl")       return new QsciLexerVHDL(parent);
    if (nppName=="ini")        return new QsciLexerProperties(parent);
    return nullptr;
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

    QString nppName  = extToNppName(ext, fileName);
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

    QsciLexer* newLexer = nppName.isEmpty() ? nullptr : createLexer(nppName, this);
    if (newLexer)
        newLexer->setDefaultFont(editorFont());

    QsciLexer* old = lexer();
    setLexer(newLexer);
    delete old;

    if (newLexer) {
        applyStylers(nppName);  // 从 stylers.xml 应用颜色
        setupAutoComplete();
    } else {
        setFont(editorFont());
    }
    applyGlobalStyles();
    scheduleUrlRefresh();
    refreshXmlTagHighlight();
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

    QsciLexer* old = lexer();
    UserDefinedLexer* custom = new UserDefinedLexer(language, this);
    custom->setDefaultFont(editorFont());
    setLexer(custom);
    delete old;
    setupAutoComplete();
    applyGlobalStyles();
}

// ─── stylers.xml 样式应用 ─────────────────────────────────────────────────────

void ScintillaEditView::applyStylers(const QString& nppLexerName)
{
    const LexerStyler* ls = NppParameters::getInstance().getLexerStyler(nppLexerName);
    if (!ls || !lexer()) return;

    NppGUI& gui = NppParameters::getInstance().getNppGUI();
    QFont   defaultFont(gui._editorFontName, gui._editorFontSize);

    for (const WordsStyle& ws : ls->styles) {
        QFont f = ws.fontName.isEmpty() ? defaultFont : QFont(ws.fontName, ws.fontSize > 0 ? ws.fontSize : gui._editorFontSize);
        if (ws.fontStyle & 1) f.setBold(true);
        if (ws.fontStyle & 2) f.setItalic(true);
        if (ws.fontStyle & 4) f.setUnderline(true);
        lexer()->setFont(f, ws.styleID);
        if (ws.hasFg) lexer()->setColor(ws.fgColor, ws.styleID);
        if (ws.hasBg) lexer()->setPaper(ws.bgColor, ws.styleID);
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
    // 同时撤销 QScintilla 构造器里 setSelectionForegroundColor(palette.highlightedText())
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

    QsciLexer* newLexer = createLexer(normalized, this);
    if (!newLexer)
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
    newLexer->setDefaultFont(editorFont());
    QsciLexer* old = lexer();
    setLexer(newLexer);
    delete old;
    applyStylers(normalized);
    setupAutoComplete();
    applyGlobalStyles();
    scheduleUrlRefresh();
    refreshXmlTagHighlight();
    return true;
}

void ScintillaEditView::clearLexer()
{
    _currentLexerName.clear();
    QsciLexer* old = lexer();
    setLexer(nullptr);
    delete old;
    setFont(editorFont());
}

// ─── 行注释切换 ───────────────────────────────────────────────────────────────

void ScintillaEditView::toggleLineComment()
{
    // 优先使用从 langs.xml 读取的行注释符
    QString commentStr = _commentLine;

    // 回退：从 QScintilla lexer 推断
    if (commentStr.isEmpty()) {
        QsciLexer* lex = lexer();
        if (!lex) return;
        QString lang = QString(lex->language()).toLower();
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
        QsciScintilla::getSelection(&fromLine, &fromIdx, &toLine, &toIdx);
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
    QsciLexer* lex = lexer();
    if (!lex) return;

    QsciAPIs* api = new QsciAPIs(lex);

    // 使用词法分析器内置的关键字集合填充 API
    for (int set = 1; set <= 9; ++set) {
        const char* kw = lex->keywords(set);
        if (!kw) continue;
        QString kwStr(kw);
        for (const QString& word : kwStr.split(' ', QString::SkipEmptyParts))
            api->add(word);
    }
    api->prepare();

    // 从文档单词和 API 两处补全
    setAutoCompletionSource(QsciScintilla::AcsAll);
    setAutoCompletionThreshold(3);
    setAutoCompletionCaseSensitivity(false);
    setAutoCompletionReplaceWord(true);
}

void ScintillaEditView::applyAutoComplete(bool enable, int threshold)
{
    if (_largeFileMode)
        enable = false;
    if (enable) {
        setAutoCompletionSource(QsciScintilla::AcsAll);
        setAutoCompletionThreshold(threshold);
    } else {
        setAutoCompletionSource(QsciScintilla::AcsNone);
    }
}

// ─── 书签 ─────────────────────────────────────────────────────────────────────

void ScintillaEditView::onMarginClicked(int margin, int line, Qt::KeyboardModifiers)
{
    if (margin == _SC_MARGE_SYMBOL)
        toggleBookmark();
}

void ScintillaEditView::toggleBookmark()
{
    int line, index;
    getCursorPosition(&line, &index);
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
    if (lexer()) {
        lexer()->setDefaultFont(f);
        setLexer(lexer());  // 刷新样式
    }
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
    setWrapMode(enable ? QsciScintilla::WrapWord : QsciScintilla::WrapNone);
}

void ScintillaEditView::applyShowWhitespace(bool show)
{
    setWhitespaceVisibility(show ? QsciScintilla::WsVisible : QsciScintilla::WsInvisible);
}

void ScintillaEditView::applyShowEol(bool show)
{
    setEolVisibility(show);
}

void ScintillaEditView::applyShowIndentGuide(bool show)
{
    // 对应原版 showIndentGuideLine()：
    // 使用 SC_IV_LOOKBOTH（原版非 Python 语言默认），而非 QScintilla 默认的 SC_IV_REAL
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
            QsciScintilla::SCI_INDICATOREND,
            static_cast<quintptr>(indicator), position);
        if (rangeEnd <= position)
            break;
        if (SendScintillaNpp(QsciScintilla::SCI_INDICATORVALUEAT,
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
    SendScintilla(SCI_GETSELTEXT, bytes.data());
    bytes.resize(selLen);

    // 多行选区 → 不高亮
    if (bytes.contains('\n') || bytes.contains('\r'))
        return;

    // 用像素坐标反查可视行范围，避免 SCI_GETFIRSTVISIBLELINE/SCI_LINESONSCREEN 在
    // QScintilla 中可能返回偏小范围（导致选区在屏幕上半部分时落在搜索范围外）的问题
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
        searchFlags |= QsciScintillaBase::SCFIND_WHOLEWORD;
    if (gui._smartHighlightMatchCase)
        searchFlags |= QsciScintillaBase::SCFIND_MATCHCASE;
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
                                             bytes.constData());
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
