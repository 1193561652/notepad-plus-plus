#pragma once

#include <QtGlobal>

#ifndef Q_OS_WIN
#error Win32EditorMessageAdapter is available only on Windows.
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "Scintilla.h"

class ScintillaEditView;

class Win32EditorMessageAdapter
{
public:
    Win32EditorMessageAdapter(ScintillaEditView* editor, HWND handle);

    ScintillaEditView* editor() const { return _editor; }
    HWND handle() const { return _handle; }
    void setSelectionTextLengthIncludesTerminator(bool enabled)
    {
        _selectionTextLengthIncludesTerminator = enabled;
    }
    void setLegacyTextRangeAbi(bool enabled)
    {
        _legacyTextRangeAbi = enabled;
    }
    bool isValid() const
        { return _editor != nullptr && _handle && IsWindow(_handle); }

    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam,
                          bool* handled) const;

private:
    static sptr_t directFunction(sptr_t pointer, unsigned int message,
                                 uptr_t wParam, sptr_t lParam);
    sptr_t sendLegacyDirectMessage(unsigned int message, uptr_t wParam,
                                   sptr_t lParam) const;

    ScintillaEditView* _editor = nullptr;
    HWND _handle = nullptr;
    bool _selectionTextLengthIncludesTerminator = false;
    bool _legacyTextRangeAbi = true;
};
