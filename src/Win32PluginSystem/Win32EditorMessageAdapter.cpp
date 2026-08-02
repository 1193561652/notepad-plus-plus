#include "Win32PluginSystem/Win32EditorMessageAdapter.h"

#include "ScintillaComponent/ScintillaEditView.h"

Win32EditorMessageAdapter::Win32EditorMessageAdapter(
    ScintillaEditView* editor, HWND handle)
    : _editor(editor), _handle(handle)
{
    Q_ASSERT(_editor);
    Q_ASSERT(_handle);
}

LRESULT Win32EditorMessageAdapter::handleMessage(
    UINT message, WPARAM wParam, LPARAM lParam, bool* handled) const
{
    if (handled)
        *handled = true;

    switch (message) {
        case SCI_GETSELECTIONSTART:
        case SCI_GETSELECTIONEND:
        case SCI_GETSELTEXT:
        case SCI_TARGETFROMSELECTION:
        case SCI_GETTARGETTEXT:
        case SCI_SETTARGETSTART:
        case SCI_SETTARGETEND:
        case SCI_REPLACETARGET:
        case SCI_SETSEL:
            return static_cast<LRESULT>(_editor->SendScintillaNpp(
                message, static_cast<uptr_t>(wParam),
                static_cast<sptr_t>(lParam)));
        default:
            if (handled)
                *handled = false;
            return 0;
    }
}
