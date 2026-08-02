#include "Win32PluginSystem/Win32SubEditorAdapter.h"

#include "ScintillaComponent/ScintillaEditView.h"

Win32SubEditorAdapter::Win32SubEditorAdapter(
    ScintillaEditView* editor, HWND handle)
    : Win32EditorMessageAdapter(editor, handle)
{}

QWidget* Win32SubEditorAdapter::window() const
{
    return editor();
}
