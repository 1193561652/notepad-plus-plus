// FileInterface.h - 保留 TinyXml 既有包装名，内部统一使用跨平台 Qt 文件写入
// 移植自: v8.4.6:PowerEditor/src/MISC/Common/FileInterface.h
#pragma once

#include <string>
#include <QFile>
#include <QString>

// TinyXml 的 SaveFile 只需要 isOpened / writeStr 两个接口
class Win32_IO_File
{
public:
    explicit Win32_IO_File(const wchar_t* fname)
        : _hFile(QString::fromWCharArray(fname))
    {
        _hFile.open(QFile::WriteOnly | QFile::Truncate);
    }

    explicit Win32_IO_File(const char* fname)
        : _hFile(QString::fromUtf8(fname))
    {
        _hFile.open(QFile::WriteOnly | QFile::Truncate);
    }

    Win32_IO_File()                                = delete;
    Win32_IO_File(const Win32_IO_File&)            = delete;
    Win32_IO_File& operator=(const Win32_IO_File&) = delete;

    ~Win32_IO_File()
    {
        if (_hFile.isOpen())
            _hFile.close();
    }

    bool isOpened() const { return _hFile.isOpen(); }

    bool writeStr(const std::string& str)
    {
        qint64 n = _hFile.write(str.c_str(), static_cast<qint64>(str.length()));
        return n == static_cast<qint64>(str.length());
    }

private:
    QFile _hFile;
};
