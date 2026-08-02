// Buffer.h - 文档缓冲区类
// 移植自: v8.4.6:PowerEditor/src/ScintillaComponent/Buffer.h

#ifndef BUFFER_H
#define BUFFER_H

#include <QString>
#include <QDateTime>
#include <QList>
#include <functional>
#include <limits>
#include "MISC/TextFileCodec.h"

class ScintillaEditView;

struct BufferMapState {
    int firstVisibleDisplayLine = -1;
    int firstVisibleDocLine = -1;
    int lastVisibleDocLine = -1;
    int lineCount = -1;
    int higherPosition = -1;
    int width = -1;
    int height = -1;
    qint64 kBytesInDocument = -1;
    int wrapIndentMode = -1;
    bool isWrap = false;
};

class Buffer
{
public:
    static constexpr qint64 LargeFileThreshold =
        qint64(200) * 1024 * 1024;

    explicit Buffer(int untitledNumber);
    ~Buffer();

    // 路径管理
    const QString& getFullPath() const { return _fullPath; }
    const QString& getFileName() const { return _fileName; }
    void setFilePath(const QString& path);
    bool isUntitled() const { return _isUntitled; }

    // 修改状态
    bool isDirty() const { return _isDirty || _metadataDirty; }
    void setDirty(bool dirty) {
        _isDirty = dirty;
        if (!dirty)
            _metadataDirty = false;
    }
    void setTextDirty(bool dirty) { _isDirty = dirty; }
    void setMetadataDirty(bool dirty) { _metadataDirty = dirty; }

    // 关联的编辑器视图
    ScintillaEditView* getView() const;
    const QList<ScintillaEditView*>& views() const { return _views; }
    void setView(ScintillaEditView* view);
    void addView(ScintillaEditView* view);
    void removeView(ScintillaEditView* view);
    bool containsView(ScintillaEditView* view) const;
    qintptr document() const { return _document; }
    void captureDocument(qintptr document, ScintillaEditView* owner,
                         std::function<void(qintptr)> releaser);
    void releaseDocument();

    // 标签页显示文本（脏状态时加 *）
    QString getTabLabel() const;

    // 编码
    const QString& getEncoding() const { return _encoding; }
    void setEncoding(const QString& enc) { _encoding = enc; }
    bool hasBom() const { return _hasBom; }
    void setHasBom(bool bom) { _hasBom = bom; }
    bool usesEncodingCookie() const { return _usesEncodingCookie; }
    void setUsesEncodingCookie(bool cookie) { _usesEncodingCookie = cookie; }
    TextEolMode getEolMode() const { return _eolMode; }
    void setEolMode(TextEolMode mode) { _eolMode = mode; }
    bool isReadOnly() const { return _isReadOnly; }
    void setReadOnly(bool readOnly) { _isReadOnly = readOnly; }
    bool isCommandLineReadOnly() const { return _commandLineReadOnly; }
    void setCommandLineReadOnly(bool readOnly) { _commandLineReadOnly = readOnly; }
    bool isMonitoring() const { return _monitoring; }
    void setMonitoring(bool monitoring) { _monitoring = monitoring; }
    bool isBinary() const { return _isBinary; }
    void setBinary(bool binary) { _isBinary = binary; }
    bool isLargeFile() const { return _isLargeFile; }
    void setLargeFile(bool largeFile) { _isLargeFile = largeFile; }
    qint64 sourceFileSize() const { return _sourceFileSize; }
    void setSourceFileSize(qint64 size) { _sourceFileSize = size; }
    static bool isLargeFileSize(
        qint64 size, qint64 threshold = LargeFileThreshold) {
        return size >= threshold;
    }
    static bool requiresHugeFileConfirmation(qint64 size) {
        if (size < 0)
            return false;
        const qint64 editingRoom = qMin<qint64>(
            qint64(1) << 20, size / 6);
        return size + editingRoom > std::numeric_limits<int>::max();
    }
    int individualTabColour() const { return _individualTabColour; }
    void setIndividualTabColour(int colour) { _individualTabColour = colour; }
    const BufferMapState& mapState() const { return _mapState; }
    void setMapState(const BufferMapState& state) { _mapState = state; }

    // 备份文件路径（与原版 sessionFileInfo::_backupFilePath 对应）
    const QString& getBackupFilePath() const { return _backupFilePath; }
    void setBackupFilePath(const QString& p)  { _backupFilePath = p; }
    void clearBackupFile(); // 删除磁盘上的备份文件并清空路径

    QDateTime lastKnownModificationTime() const { return _lastKnownModificationTime; }
    void setLastKnownModificationTime(const QDateTime& t) { _lastKnownModificationTime = t; }

private:
    QString _fullPath;
    QString _fileName;
    bool _isDirty = false;
    bool _metadataDirty = false;
    bool _isUntitled = true;
    QList<ScintillaEditView*> _views;
    qintptr _document = 0;
    ScintillaEditView* _documentOwner = nullptr;
    std::function<void(qintptr)> _documentReleaser;
    int _untitledNumber;
    QString _encoding = "UTF-8";
    bool    _hasBom   = false;
    bool _usesEncodingCookie = true;
    TextEolMode _eolMode = TextEolMode::Unknown;
    bool _isReadOnly = false;
    bool _commandLineReadOnly = false;
    bool _monitoring = false;
    bool _isBinary = false;
    bool _isLargeFile = false;
    qint64 _sourceFileSize = 0;
    int _individualTabColour = -1;
    BufferMapState _mapState;
    QString _backupFilePath; // %APPDATA%\Notepad++\backup\filename@timestamp
    QDateTime _lastKnownModificationTime;
};

using BufferID = Buffer*;
static const BufferID BUFFER_INVALID = nullptr;

#endif // BUFFER_H
