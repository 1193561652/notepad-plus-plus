#pragma once

#include "Win32PluginSystem/Win32EditorMessageAdapter.h"

#ifndef Q_OS_WIN
#error Win32SubEditorAdapter is available only on Windows.
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

class ScintillaEditView;
class QWidget;

class Win32SubEditorAdapter final : public Win32EditorMessageAdapter
{
public:
    Win32SubEditorAdapter(ScintillaEditView* editor, HWND handle);

    QWidget* window() const;
};
