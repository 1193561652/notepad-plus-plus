#pragma once

#include <QByteArray>
#include <QString>

class MainWindow;
class QMainWindow;
class ScintillaEditView;

struct PluginHostEnvironment
{
    int systemType = 0;
    int cpuArchitecture = 0;
    QString systemName;
    QString systemVersion;
    QString applicationName;
    QString applicationVersion;
};

class PluginHostServices
{
public:
    virtual ~PluginHostServices() = default;

    virtual PluginHostEnvironment environment() const = 0;
    virtual QString pluginHomePath() const = 0;
    virtual QString pluginConfigPath() const = 0;
    virtual QString currentFilePath() const = 0;
    virtual bool openFile(const QString& path) = 0;
    virtual bool isDarkModeEnabled() const = 0;

    virtual QMainWindow* mainWindow() const = 0;
    virtual ScintillaEditView* currentView() const = 0;
    virtual bool executeMenuCommand(int commandId) = 0;
    virtual bool saveCurrentFileAs(const QString& path, bool asCopy) = 0;
    virtual bool saveCurrentSession(const QString& path) = 0;
    virtual bool loadSession(const QString& path) = 0;
    virtual bool setCurrentLanguageType(int languageType) = 0;
    virtual quintptr currentBufferId() const = 0;
    virtual QString pathForBuffer(quintptr bufferId) const = 0;
    virtual int positionForBuffer(quintptr bufferId, int priorityView) const = 0;
    virtual int openFileCount(int scope) const = 0;
    virtual int currentDocumentIndex(int view) const = 0;
    virtual quintptr bufferIdAt(int, int) const { return 0; }
    virtual int currentLanguageType() const { return 0; }
    virtual bool activateDocument(int view, int index) = 0;
    virtual int currentLine() const = 0;
    virtual int bufferEncoding(quintptr bufferId) const = 0;
    virtual bool setBufferEncoding(quintptr bufferId, int encoding) = 0;
    virtual void setStatusBarText(int section, const QString& text) = 0;
    virtual bool addToolbarCommand(int commandId) = 0;
    virtual QByteArray currentDocumentBytes() const = 0;
    virtual bool replaceCurrentDocument(const QByteArray& data) = 0;
    virtual QByteArray currentSelectionBytes(qint64* start,
                                             qint64* end) const = 0;
    virtual bool replaceCurrentSelection(const QByteArray& data) = 0;
    virtual bool setCurrentSelection(qint64 start, qint64 end) = 0;
    virtual bool createDocument(const QByteArray& data) = 0;
    virtual int currentViewIndex() const = 0;
    virtual QString clipboardText() const = 0;
    virtual bool setClipboardText(const QString& text) = 0;
    virtual QByteArray viewDocumentBytes(int) const { return {}; }
    virtual bool showBufferInView(quintptr, int) { return false; }
    virtual void clearCompareMarks(int) {}
    virtual bool addCompareMark(int, qint64, quint32) { return false; }
    virtual qint64 firstVisibleLine(int) const { return -1; }
    virtual bool setFirstVisibleLine(int, qint64) { return false; }
    virtual bool gotoLine(int, qint64) { return false; }
    virtual qintptr sendScintilla(int, quint32, quintptr, qintptr) { return 0; }
    virtual bool saveCurrentFile() { return false; }
};

class MainWindowPluginHostServices final : public PluginHostServices
{
public:
    explicit MainWindowPluginHostServices(MainWindow* mainWindow);

    PluginHostEnvironment environment() const override;
    QString pluginHomePath() const override;
    QString pluginConfigPath() const override;
    QString currentFilePath() const override;
    bool openFile(const QString& path) override;
    bool isDarkModeEnabled() const override;

    QMainWindow* mainWindow() const override;
    ScintillaEditView* currentView() const override;
    bool executeMenuCommand(int commandId) override;
    bool saveCurrentFileAs(const QString& path, bool asCopy) override;
    bool saveCurrentSession(const QString& path) override;
    bool loadSession(const QString& path) override;
    bool setCurrentLanguageType(int languageType) override;
    quintptr currentBufferId() const override;
    QString pathForBuffer(quintptr bufferId) const override;
    int positionForBuffer(quintptr bufferId, int priorityView) const override;
    int openFileCount(int scope) const override;
    int currentDocumentIndex(int view) const override;
    quintptr bufferIdAt(int view, int index) const override;
    int currentLanguageType() const override;
    bool activateDocument(int view, int index) override;
    int currentLine() const override;
    int bufferEncoding(quintptr bufferId) const override;
    bool setBufferEncoding(quintptr bufferId, int encoding) override;
    void setStatusBarText(int section, const QString& text) override;
    bool addToolbarCommand(int commandId) override;
    QByteArray currentDocumentBytes() const override;
    bool replaceCurrentDocument(const QByteArray& data) override;
    QByteArray currentSelectionBytes(qint64* start, qint64* end) const override;
    bool replaceCurrentSelection(const QByteArray& data) override;
    bool setCurrentSelection(qint64 start, qint64 end) override;
    bool createDocument(const QByteArray& data) override;
    int currentViewIndex() const override;
    QString clipboardText() const override;
    bool setClipboardText(const QString& text) override;
    QByteArray viewDocumentBytes(int view) const override;
    bool showBufferInView(quintptr bufferId, int view) override;
    void clearCompareMarks(int view) override;
    bool addCompareMark(int view, qint64 line, quint32 kind) override;
    qint64 firstVisibleLine(int view) const override;
    bool setFirstVisibleLine(int view, qint64 line) override;
    bool gotoLine(int view, qint64 line) override;
    qintptr sendScintilla(int view, quint32 message,
                          quintptr wParam, qintptr lParam) override;
    bool saveCurrentFile() override;

private:
    MainWindow* _mainWindow = nullptr;
};
