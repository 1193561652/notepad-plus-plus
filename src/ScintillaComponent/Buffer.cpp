// Buffer.cpp - 文档缓冲区实现
// 移植自: v8.4.6:PowerEditor/src/ScintillaComponent/Buffer.cpp

#include "Buffer.h"
#include <QFileInfo>
#include <QFile>

Buffer::Buffer(int untitledNumber)
    : _untitledNumber(untitledNumber)
{
    _fileName = QString("new %1").arg(untitledNumber);
    _fullPath = _fileName;
}

void Buffer::setFilePath(const QString& path)
{
    _fullPath = path;
    _fileName = QFileInfo(path).fileName();
    _isUntitled = false;
}

QString Buffer::getTabLabel() const
{
    return isDirty() ? _fileName + " *" : _fileName;
}

ScintillaEditView* Buffer::getView() const
{
    return _views.isEmpty() ? nullptr : _views.first();
}

void Buffer::setView(ScintillaEditView* view)
{
    _views.clear();
    addView(view);
}

void Buffer::addView(ScintillaEditView* view)
{
    if (view && !_views.contains(view))
        _views.append(view);
}

void Buffer::removeView(ScintillaEditView* view)
{
    _views.removeAll(view);
}

bool Buffer::containsView(ScintillaEditView* view) const
{
    return _views.contains(view);
}

void Buffer::clearBackupFile()
{
    if (!_backupFilePath.isEmpty()) {
        QFile::remove(_backupFilePath);
        _backupFilePath.clear();
    }
}
