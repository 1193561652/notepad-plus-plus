#include "Win32PluginSystem/Win32MainEditorAdapter.h"

#include "ScintillaComponent/ScintillaEditView.h"

Win32MainEditorAdapter::Win32MainEditorAdapter(
    ScintillaEditView* editor, HWND handle)
    : Win32EditorMessageAdapter(editor, handle)
{}

QWidget* Win32MainEditorAdapter::window() const
{
    return editor();
}
