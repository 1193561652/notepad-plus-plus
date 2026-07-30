// ScintillaEditView.h - Scintilla 编辑器视图包装类
// 移植自: v8.4.6:PowerEditor/src/ScintillaComponent/ScintillaEditView.h

#ifndef SCINTILLAEDITVIEW_H
#define SCINTILLAEDITVIEW_H

#include <Qsci/qsciscintilla.h>
#include <Qsci/qsciscintillabase.h>
#include <QByteArray>
#include <QString>
#include <QVector>
#include "Parameters.h"

// 前向声明
class QsciLexer;
class QTimer;

/**
 * ScintillaEditView - QScintilla 的包装类
 *
 * 这个类封装了 QScintilla 编辑器，提供与原生 Notepad++ 兼容的接口。
 * 保留了 execute() 方法以便与原始代码保持一致性。
 */
class ScintillaEditView : public QsciScintilla
{
    Q_OBJECT

public:
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
    long execute(unsigned int msg, unsigned long wParam = 0, long lParam = 0) const {
        return SendScintilla(msg, wParam, lParam);
    }

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
    void setLexerForFile(const QString& filePath);
    void setBuiltinLanguage(const QString& languageName);
    void setLexerByExtension(const QString& ext);
    bool setLexerByName(const QString& name);
    static QString builtinLexerName(const QString& nppLanguageName);
    void setUserDefinedLanguage(const UserLangDesc& language);
    void clearLexer();

    // 行注释切换
    void toggleLineComment();
    void blockComment();
    void blockUncomment();

    // 书签
    void toggleBookmark();
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
    bool doNppFind();

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
    QString _currentLexerName;
    QVector<QPair<qintptr, qintptr>> _urlRanges;
    QTimer* _urlRefreshTimer = nullptr;

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

#endif // SCINTILLAEDITVIEW_H
