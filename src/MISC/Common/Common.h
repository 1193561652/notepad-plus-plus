// Common.h - 精简版，仅为 TinyXml 提供 TCHAR / generic_string 支持
// 移植自: v8.4.6:PowerEditor/src/MISC/Common/Common.h
#pragma once

#include <string>
#include <tchar.h>  // Windows TCHAR (wchar_t when UNICODE defined, MinGW 支持)
#include <cwchar>
#include <cstdlib>

// Notepad++ 约定：XML 使用宽字符路径
typedef std::basic_string<TCHAR> generic_string;

#ifdef UNICODE
#  define generic_atoi      _wtoi
#  define generic_atof      _wtof
#  define generic_fopen     _wfopen
#  define generic_fgets     fgetws
#  define generic_sprintf   swprintf
#  define generic_sscanf    swscanf
#  define generic_strncmp   wcsncmp
#  define generic_strchr    wcschr
#  define generic_strtol    wcstol
#else
#  define generic_atoi      atoi
#  define generic_atof      atof
#  define generic_fopen     fopen
#  define generic_fgets     fgets
#  define generic_sprintf   sprintf
#  define generic_sscanf    sscanf
#  define generic_strncmp   strncmp
#  define generic_strchr    strchr
#  define generic_strtol    strtol
#endif

// UTF-8 code page (Windows constant, needed by TinyXml)
#ifndef CP_UTF8
#  define CP_UTF8 65001
#endif

// wstring2string: convert wstring to narrow string using given code page
// Used by TinyXml for UTF-8 XML output.
#include <cstring>
inline std::string wstring2string(const std::wstring& ws, unsigned int /*codepage*/)
{
    if (ws.empty()) return std::string();
    // Simple UTF-16 to UTF-8 conversion via wcstombs (locale-dependent)
    // For proper UTF-8 output, convert wchar_t -> UTF-8 manually:
    std::string result;
    result.reserve(ws.size() * 3);
    for (wchar_t wc : ws) {
        unsigned int cp = static_cast<unsigned int>(wc);
        if (cp < 0x80) {
            result += static_cast<char>(cp);
        } else if (cp < 0x800) {
            result += static_cast<char>(0xC0 | (cp >> 6));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            result += static_cast<char>(0xE0 | (cp >> 12));
            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
    return result;
}
