// Common.h - 精简版，仅为 TinyXml 提供 TCHAR / generic_string 支持
// 移植自: v8.4.6:PowerEditor/src/MISC/Common/Common.h
#pragma once

#include <string>
#include <cwchar>
#include <cwctype>
#include <cstdlib>
#include <QByteArray>
#include <QString>

// The port always uses the original Unicode TinyXml model on every platform.
using TCHAR = wchar_t;
using generic_string = std::wstring;

inline int generic_atoi(const wchar_t* value)
{
    return static_cast<int>(std::wcstol(value, nullptr, 10));
}

inline double generic_atof(const wchar_t* value)
{
    return std::wcstod(value, nullptr);
}

template <typename... Args>
inline int generic_sscanf(const wchar_t* buffer, const wchar_t* format,
                          Args... args)
{
    return std::swscanf(buffer, format, args...);
}

inline int generic_strncmp(const wchar_t* lhs, const wchar_t* rhs,
                           size_t count)
{
    return std::wcsncmp(lhs, rhs, count);
}

inline const wchar_t* generic_strchr(const wchar_t* value, wchar_t character)
{
    return std::wcschr(value, character);
}

inline long generic_strtol(const wchar_t* value, wchar_t** end, int base)
{
    return std::wcstol(value, end, base);
}

// Keep the existing conversion API used by TinyXml. Qt handles both 16-bit
// Windows wchar_t surrogate pairs and 32-bit Unix wchar_t code points consistently.
constexpr unsigned int CP_UTF8 = 65001;

inline std::string wstring2string(const std::wstring& ws,
                                 unsigned int /*codepage*/)
{
    const QByteArray utf8 = QString::fromStdWString(ws).toUtf8();
    return std::string(utf8.constData(), static_cast<size_t>(utf8.size()));
}
