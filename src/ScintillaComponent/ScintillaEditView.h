// ScintillaEditView.h - Scintilla 编辑器视图包装类
// 移植自: v8.4.6:PowerEditor/src/ScintillaComponent/ScintillaEditView.h

#ifndef SCINTILLAEDITVIEW_H
#define SCINTILLAEDITVIEW_H

#include <ScintillaEditBase.h>
#include <Scintilla.h>
#include <SciLexer.h>
#include <ILexer.h>
#include <QByteArray>
#include <QColor>
#include <QFont>
#include <QString>
#include <QStringList>
#include <QVector>
#include "Parameters.h"

// 前向声明
class QTimer;

/**
 * ScintillaEditView - Scintilla 5 Qt 编辑器包装类
 *
 * 这个类封装官方 ScintillaEditBase，并提供与原生 Notepad++ 兼容的接口。
 * 保留了 execute() 方法以便与原始代码保持一致性。
 */
class ScintillaEditView : public ScintillaEditBase
{
    Q_OBJECT

public:
    using ExternalLexerFactory = Scintilla::ILexer5* (*)(const char* name);
    enum EolMode { EolWindows = SC_EOL_CRLF, EolUnix = SC_EOL_LF, EolMac = SC_EOL_CR };
    enum WrapMode { WrapNone = SC_WRAP_NONE, WrapWord = SC_WRAP_WORD };
    enum WhitespaceVisibility { WsInvisible = SCWS_INVISIBLE, WsVisible = SCWS_VISIBLEALWAYS };
    enum AutoCompletionSource { AcsNone, AcsAll };
    enum BraceMatch { NoBraceMatch, SloppyBraceMatch };

    explicit ScintillaEditView(QWidget *parent = nullptr);
    ~ScintillaEditView();

    /**
     * execute() - 执行 Scintilla 命令
     *
     * 这个方法保持与原生 Notepad++ 的兼容性，内部调用 SendScintilla()
     *
     * @param msg - Scintilla 消息 ID (SCI_*)
     * @param wParam - 第一个参数
     * @param lParam - 第二个参数
     * @return 命令执行结果
     */
    sptr_t execute(unsigned int msg, uptr_t wParam = 0, sptr_t lParam = 0) const
        { return send(msg, wParam, lParam); }
    sptr_t SendScintilla(unsigned int msg, uptr_t wParam = 0, sptr_t lParam = 0) const
        { return send(msg, wParam, lParam); }
    sptr_t SendScintillaNpp(unsigned int msg, uptr_t wParam = 0, sptr_t lParam = 0) const
        { return send(msg, wParam, lParam); }
    sptr_t SendScintillaPtrResult(unsigned int msg, uptr_t wParam = 0, sptr_t lParam = 0) const
        { return send(msg, wParam, lParam); }

    sptr_t document() const { return send(SCI_GETDOCPOINTER); }
    void setDocument(sptr_t pointer) { send(SCI_SETDOCPOINTER, 0, pointer); }
    int length() const { return static_cast<int>(send(SCI_GETLENGTH)); }
    int lines() const { return static_cast<int>(send(SCI_GETLINECOUNT)); }
    bool hasSelectedText() const { return send(SCI_GETSELECTIONEMPTY) == 0; }
    QString text() const { return getText(); }
    QString text(int line) const;
    QString text(int start, int end) const;
    QString selectedText() const;
    void replaceSelectedText(const QString& text);
    void removeSelectedText() { send(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>("")); }
    void insert(const QString& text);
    void insertAt(const QString& text, int line, int index);
    void append(const QString& text) { appendText(text); }
    void beginUndoAction() { send(SCI_BEGINUNDOACTION); }
    void endUndoAction() { send(SCI_ENDUNDOACTION); }
    void setReadOnly(bool value) { send(SCI_SETREADONLY, value); }
    bool isReadOnly() const { return send(SCI_GETREADONLY) != 0; }
    bool isModified() const { return send(SCI_GETMODIFY) != 0; }
    void setEolMode(EolMode mode) { send(SCI_SETEOLMODE, mode); }
    EolMode eolMode() const { return static_cast<EolMode>(send(SCI_GETEOLMODE)); }
    void convertEols(EolMode mode) { send(SCI_CONVERTEOLS, mode); }
    void setWrapMode(WrapMode mode) { send(SCI_SETWRAPMODE, mode); }
    WrapMode wrapMode() const { return static_cast<WrapMode>(send(SCI_GETWRAPMODE)); }
    void setWhitespaceVisibility(WhitespaceVisibility mode) { send(SCI_SETVIEWWS, mode); }
    WhitespaceVisibility whitespaceVisibility() const
        { return static_cast<WhitespaceVisibility>(send(SCI_GETVIEWWS)); }
    void setEolVisibility(bool visible) { send(SCI_SETVIEWEOL, visible); }
    void setTabWidth(int width) { send(SCI_SETTABWIDTH, width); }
    void setIndentationsUseTabs(bool value) { send(SCI_SETUSETABS, value); }
    void setAutoIndent(bool value) { _autoIndent = value; }
    void setBraceMatching(BraceMatch mode) { _braceMatch = mode; }
    void setBorderEdge(bool enabled, bool darkMode);
    void setMarginWidth(int margin, int width) { send(SCI_SETMARGINWIDTHN, margin, width); }
    void setMarginWidth(int margin, const QString& sample);
    int marginWidth(int margin) const { return static_cast<int>(send(SCI_GETMARGINWIDTHN, margin)); }
    void setMarginSensitivity(int margin, bool value) { send(SCI_SETMARGINSENSITIVEN, margin, value); }
    void setMarginType(int margin, int type) { send(SCI_SETMARGINTYPEN, margin, type); }
    void ensureLineVisible(int line) { send(SCI_ENSUREVISIBLE, line); }
    int positionFromLineIndex(int line, int index) const
        { return static_cast<int>(send(SCI_FINDCOLUMN, line, index)); }
    void lineIndexFromPosition(int position, int* line, int* index) const;
    int lineLength(int line) const { return static_cast<int>(send(SCI_LINELENGTH, line)); }
    int firstVisibleLine() const { return static_cast<int>(send(SCI_GETFIRSTVISIBLELINE)); }
    void setFirstVisibleLine(int line) { send(SCI_SETFIRSTVISIBLELINE, line); }
    void setCursorPosition(int line, int index);
    void getCursorPosition(int* line, int* index) const;
    void setSelection(int lineFrom, int indexFrom, int lineTo, int indexTo);
    void getSelection(int* lineFrom, int* indexFrom, int* lineTo, int* indexTo) const;
    int markerAdd(int line, int marker) { return static_cast<int>(send(SCI_MARKERADD, line, marker)); }
    void markerDelete(int line, int marker) { send(SCI_MARKERDELETE, line, marker); }
    void markerDeleteAll(int marker) { send(SCI_MARKERDELETEALL, marker); }
    int markersAtLine(int line) const { return static_cast<int>(send(SCI_MARKERGET, line)); }
    int markerFindNext(int line, unsigned mask) const { return static_cast<int>(send(SCI_MARKERNEXT, line, mask)); }
    int markerFindPrevious(int line, unsigned mask) const { return static_cast<int>(send(SCI_MARKERPREVIOUS, line, mask)); }
    void markerDefine(int symbol, int marker) { send(SCI_MARKERDEFINE, marker, symbol); }
    void setAutoCompletionSource(AutoCompletionSource source) { _autoCompletionSource = source; }
    AutoCompletionSource autoCompletionSource() const { return _autoCompletionSource; }
    void setAutoCompletionThreshold(int value) { _autoCompletionThreshold = value; }
    void setAutoCompletionCaseSensitivity(bool value) { send(SCI_AUTOCSETIGNORECASE, !value); }
    void setAutoCompletionReplaceWord(bool value) { send(SCI_AUTOCSETDROPRESTOFWORD, value); }
    void autoCompleteFromAPIs();
    void autoCompleteFromDocument();
    void callTip();
    QString lexerLanguage() const { return _currentLexerName; }
    bool hasLexer() const { return _lexerInstalled; }
    QString lexerKeywordSet(int slot) const
        { return slot >= 0 && slot < _installedKeywordSets.size()
            ? _installedKeywordSets[slot] : QString(); }
    void setFolding(int style);
    void setFont(const QFont& font);
    void setCaretForegroundColor(const QColor& color) { send(SCI_SETCARETFORE, sciColour(color)); }
    void setCaretLineBackgroundColor(const QColor& color) { send(SCI_SETCARETLINEBACK, sciColour(color)); }
    void setCaretLineVisible(bool value) { send(SCI_SETCARETLINEVISIBLE, value); }
    void setCaretWidth(int width) { send(SCI_SETCARETWIDTH, width); }
    void setMarginsForegroundColor(const QColor& color) { send(SCI_STYLESETFORE, STYLE_LINENUMBER, sciColour(color)); }
    void setMarginsBackgroundColor(const QColor& color) { send(SCI_STYLESETBACK, STYLE_LINENUMBER, sciColour(color)); }
    void setMarkerBackgroundColor(const QColor& color, int marker) { send(SCI_MARKERSETBACK, marker, sciColour(color)); }
    void setMarkerForegroundColor(const QColor& color, int marker) { send(SCI_MARKERSETFORE, marker, sciColour(color)); }
    void setSelectionBackgroundColor(const QColor& color) { send(SCI_SETSELBACK, 1, sciColour(color)); }
    void resetSelectionForegroundColor() { send(SCI_SETSELFORE, 0, 0); }
    void selectAll() { send(SCI_SELECTALL); }
    void undo() { send(SCI_UNDO); }
    void redo() { send(SCI_REDO); }
    void cut() { send(SCI_CUT); }
    void copy() { send(SCI_COPY); }
    void paste() { send(SCI_PASTE); }
    bool isUndoAvailable() const { return send(SCI_CANUNDO) != 0; }
    bool isRedoAvailable() const { return send(SCI_CANREDO) != 0; }
    void foldLine(int line) { send(SCI_TOGGLEFOLD, line); }
    void zoomIn() { send(SCI_ZOOMIN); }
    void zoomOut() { send(SCI_ZOOMOUT); }
    void zoomTo(int level) { send(SCI_SETZOOM, level); }
    QString wordAtLineIndex(int line, int index) const;

signals:
    void textChanged();
    void cursorPositionChanged(int line, int index);

public:
    // 基础文本操作
    void setText(const QString &text);
    QString getText() const;
    void appendText(const QString &text);
    void insertText(int pos, const QString &text);

    // Notepad++ large-file document and pointer-width I/O helpers.
    bool createLargeDocument();
    void createStandardDocument();
    qintptr documentLengthNpp() const;
    bool appendUtf8Chunk(const QByteArray& bytes);
    const char* utf8RangePointer(qintptr position, qintptr length) const;
    qintptr gapPositionNpp() const;
    void beginBulkLoad(qint64 expectedBytes);
    bool endBulkLoad();
    void setLargeFileMode(bool enabled);
    bool isLargeFileMode() const { return _largeFileMode; }

    // 光标和选择
    int getCurrentPos() const;
    void setCurrentPos(int pos);
    void getSelection(int *startPos, int *endPos) const;
    qintptr currentPositionNpp() const;
    void setCurrentPositionNpp(qintptr position);
    void getSelectionNpp(qintptr* startPos, qintptr* endPos) const;

    bool findFirst(const QString& expression, bool regex, bool matchCase,
                   bool wholeWord, bool wrap, bool forward = true,
                   int line = -1, int index = -1, bool show = true,
                   bool posix = false, bool cxx11 = false);
    bool findNext();
    void replace(const QString& replacement);

    // 初始化编辑器
    void init();

    // 语法高亮
    void setLexerForFile(const QString& filePath,
                         const QString& detectedLanguage = QString());
    void setBuiltinLanguage(const QString& languageName);
    void setLexerByExtension(const QString& ext);
    bool setLexerByName(const QString& name);
    bool installExternalLexer(const QString& name,
                              ExternalLexerFactory factory);
    static QString builtinLexerName(const QString& nppLanguageName);
    void setUserDefinedLanguage(const UserLangDesc& language);
    void clearLexer();

    // 行注释切换
    void toggleLineComment();
    void blockComment();
    void blockUncomment();

    // 书签
    void toggleBookmark();
    void toggleBookmark(int line);
    void nextBookmark();
    void prevBookmark();
    void clearAllBookmarks();

    // 偏好设置应用
    void applyFont(const QString& family, int size);
    void applyTabSettings(int width, bool useSpaces);
    void applyAutoComplete(bool enable, int threshold = 3);
    void applyWordWrap(bool enable);
    void applyShowWhitespace(bool show);
    void applyShowEol(bool show);
    void applyShowIndentGuide(bool show);   // 对应原版 showIndentGuideLine()
    void refreshUrlHotspots();
    void refreshXmlTagHighlight();
    QString markedText(int indicator) const;

    // 从 stylers.xml 应用颜色样式（在设置 lexer 后调用）
    void applyStylers(const QString& nppLexerName);
    // 应用全局样式（编辑器背景、行号边距等）
    void applyGlobalStyles();
    void reloadConfiguredStyles();

    // 当前行注释字符（从 langs.xml 读取，setLexerForFile 后有效）
    const QString& commentLine()  const { return _commentLine;  }
    const QString& commentStart() const { return _commentStart; }
    const QString& commentEnd()   const { return _commentEnd;   }

    // 公开指示器编号常量，供 FindReplaceDlg 等外部类使用
    // Smart Highlighting 指示器号（对应原版 SCE_UNIVERSAL_FOUND_STYLE_SMART）
    static const int SMART_HIGHLIGHT_INDICATOR = 29;
    // Find All / Mark All 高亮指示器号（对应原版 SCE_UNIVERSAL_FOUND_STYLE = 31）
    static const int FIND_MARK_INDICATOR       = 31;

private slots:
    void onMarginClicked(int margin, int line, Qt::KeyboardModifiers state);
    void updateSmartHighlight(int updated);   // Smart Highlighting（SCN_UPDATEUI 驱动）
    void onCharacterAdded(int character);
    void onIndicatorClicked(int position, int modifiers);

private:
    static int sciColour(const QColor& color)
        { return color.red() | (color.green() << 8) | (color.blue() << 16); }
    bool doNppFind();
    bool installLexer(const QString& lexerName, const LangDesc* language = nullptr);
    QStringList configuredKeywords(const QString& languageName) const;
    void configureLexer(const QString& languageName);
    void setKeywordSlot(int slot, const QString& keywords);
    void setLexerProperty(const char* name, const char* value);

    void setupDefaultStyles();
    void setupMargins();
    void setupIndicators();
    void setupAutoComplete();
    void scheduleUrlRefresh();

private:
    QString _commentLine;   // 当前语言的行注释符
    QString _commentStart;  // 块注释起始
    QString _commentEnd;    // 块注释结束
    bool _largeFileMode = false;
    bool _autoPairEditing = false;
    bool _lexerInstalled = false;
    bool _autoIndent = true;
    BraceMatch _braceMatch = SloppyBraceMatch;
    AutoCompletionSource _autoCompletionSource = AcsNone;
    int _autoCompletionThreshold = -1;
    QString _currentLexerName;
    QVector<QPair<qintptr, qintptr>> _urlRanges;
    QTimer* _urlRefreshTimer = nullptr;
    QStringList _completionWords;
    QStringList _completionCallTips;
    QStringList _installedKeywordSets;

    struct SearchState {
        QString expression;
        int flags = 0;
        qintptr start = 0;
        qintptr end = 0;
        bool wrap = false;
        bool forward = true;
        bool show = true;
        bool active = false;
    } _searchState;

    // 边距常量
    static const int _SC_MARGE_LINENUMBER = 0;
    static const int _SC_MARGE_SYMBOL     = 1;
    static const int _SC_MARGE_FOLDER     = 2;
    // 书签标记号
    static const int BOOKMARK_MARKER           = 1;
    static const int URL_INDICATOR              = 27;
    static const int XML_TAG_INDICATOR          = 28;
};

using EolMode = ScintillaEditView::EolMode;
constexpr EolMode EolWindows = ScintillaEditView::EolWindows;
constexpr EolMode EolUnix = ScintillaEditView::EolUnix;
constexpr EolMode EolMac = ScintillaEditView::EolMac;
constexpr auto WrapNone = ScintillaEditView::WrapNone;
constexpr auto WrapWord = ScintillaEditView::WrapWord;
constexpr auto WsVisible = ScintillaEditView::WsVisible;
constexpr auto WsInvisible = ScintillaEditView::WsInvisible;
constexpr int NumberMargin = SC_MARGIN_NUMBER;
constexpr int SymbolMargin = SC_MARGIN_SYMBOL;
constexpr int ColourMargin = SC_MARGIN_COLOUR;
constexpr int Circle = SC_MARK_CIRCLE;
constexpr int BoxedTreeFoldStyle = 1;

#ifndef SCFIND_REGEXP_DOTMATCHESNL
#define SCFIND_REGEXP_DOTMATCHESNL 0x10000000
#define SCFIND_REGEXP_EMPTYMATCH_ALL 0x40000000
#define SCFIND_REGEXP_SKIPCRLFASONE 0x08000000
#endif

#endif // SCINTILLAEDITVIEW_H
