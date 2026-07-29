// FileManager.h - 文件管理器（单例）
// 移植自: v8.4.6:PowerEditor/src/MISC/

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QVector>
#include <QString>
#include "ScintillaComponent/Buffer.h"

class FileManager
{
public:
    static FileManager& getInstance();

    // 创建新的未命名缓冲区
    Buffer* newBuffer();

    // 从文件加载缓冲区，失败返回 nullptr
    Buffer* loadBuffer(const QString& filePath);
    bool loadBufferContent(Buffer* buf, ScintillaEditView* view,
                           const TextDecodingOptions& options,
                           const QString& forcedEncoding = QString(),
                           QString* errorMessage = nullptr,
                           const QString& sourcePath = QString());

    // 保存缓冲区到指定路径，成功返回 true
    bool saveBuffer(Buffer* buf, const QString& filePath,
                    QString* errorMessage = nullptr);
    bool saveBufferCopy(const Buffer* buf, const QString& filePath,
                        QString* errorMessage = nullptr) const;
    static bool backupFileBeforeSave(const QString& sourcePath, int mode,
                                     bool useCustomDirectory,
                                     const QString& customDirectory,
                                     const QString& timestamp,
                                     QString* backupPath = nullptr,
                                     QString* errorMessage = nullptr);

    // 关闭并销毁缓冲区
    void closeBuffer(Buffer* buf);

    // 检查路径是否已打开，返回对应缓冲区，否则返回 nullptr
    Buffer* findBufferByPath(const QString& filePath) const;
    Buffer* findBufferByView(const ScintillaEditView* view) const;
    const QVector<Buffer*>& buffers() const { return _buffers; }

private:
    FileManager() = default;
    FileManager(const FileManager&) = delete;
    FileManager& operator=(const FileManager&) = delete;

    QVector<Buffer*> _buffers;
    int _untitledCounter = 0;
};

#define MainFileManager FileManager::getInstance()

#endif // FILEMANAGER_H
