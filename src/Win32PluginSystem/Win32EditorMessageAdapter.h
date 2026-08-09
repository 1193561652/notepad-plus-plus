#pragma once

#include <QtGlobal>

#ifndef Q_OS_WIN
#error Win32EditorMessageAdapter is available only on Windows.
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

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
    bool isValid() const
        { return _editor != nullptr && _handle && IsWindow(_handle); }

    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam,
                          bool* handled) const;

private:
    ScintillaEditView* _editor = nullptr;
    HWND _handle = nullptr;
    bool _selectionTextLengthIncludesTerminator = false;
};
