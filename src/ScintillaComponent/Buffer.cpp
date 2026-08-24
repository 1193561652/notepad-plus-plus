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

Buffer::~Buffer()
{
    releaseDocument();
}

QString Buffer::detectLanguageFromTextBeginning(const QByteArray& data)
{
    if (data.size() <= 3)
        return QString();

    int offset = data.startsWith("\xEF\xBB\xBF") ? 3 : 0;
    while (offset < data.size()) {
        const char ch = data.at(offset);
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r')
            break;
        ++offset;
    }

    QByteArray firstLine = data.mid(offset, 40);
    const int cr = firstLine.indexOf('\r');
    const int lf = firstLine.indexOf('\n');
    int end = firstLine.size();
    if (cr >= 0) end = qMin(end, cr);
    if (lf >= 0) end = qMin(end, lf);
    firstLine.truncate(end);

    if (firstLine.startsWith("#!")) {
        // 顺序与 Notepad++ v8.4.6 原版保持一致。
        const struct { const char* pattern; const char* language; } shebangs[] = {
            {"sh", "bash"}, {"python", "python"}, {"perl", "perl"},
            {"php", "php"}, {"ruby", "ruby"}, {"node", "javascript"}
        };
        for (const auto& entry : shebangs) {
            if (firstLine.contains(entry.pattern))
                return QString::fromLatin1(entry.language);
        }
        return QString();
    }

    const struct { const char* pattern; const char* language; } markers[] = {
        {"<?xml", "xml"}, {"<?php", "php"}, {"<html", "html"},
        {"<!DOCTYPE html", "html"}, {"<?", "php"}
    };
    for (const auto& entry : markers) {
        if (firstLine.startsWith(entry.pattern))
            return QString::fromLatin1(entry.language);
    }
    return QString();
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

void Buffer::captureDocument(
    qintptr document, ScintillaEditView* owner,
    std::function<void(qintptr)> releaser)
{
    if (_document == document) {
        addView(owner);
        return;
    }
    releaseDocument();
    _document = document;
    _documentOwner = owner;
    _documentReleaser = std::move(releaser);
    addView(owner);
}

void Buffer::releaseDocument()
{
    if (_document && _documentReleaser)
        _documentReleaser(_document);
    _document = 0;
    _documentOwner = nullptr;
    _documentReleaser = {};
}

void Buffer::clearBackupFile()
{
    if (!_backupFilePath.isEmpty()) {
        QFile::remove(_backupFilePath);
        _backupFilePath.clear();
    }
}
