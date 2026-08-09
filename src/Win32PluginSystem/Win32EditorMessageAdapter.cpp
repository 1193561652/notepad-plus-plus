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
        case SCI_BEGINUNDOACTION:
        case SCI_ANNOTATIONCLEARALL:
        case SCI_ANNOTATIONGETSTYLES:
        case SCI_ANNOTATIONGETTEXT:
        case SCI_ANNOTATIONSETSTYLES:
        case SCI_ANNOTATIONSETTEXT:
        case SCI_ANNOTATIONSETVISIBLE:
        case SCI_ADDTEXT:
        case SCI_APPENDTEXT:
        case SCI_BRACEHIGHLIGHT:
        case SCI_CALLTIPCANCEL:
        case SCI_CALLTIPSHOW:
        case SCI_CLEARALL:
        case SCI_CLEARSELECTIONS:
        case SCI_EMPTYUNDOBUFFER:
        case SCI_ENDUNDOACTION:
        case SCI_ENSUREVISIBLE:
        case SCI_FINDCOLUMN:
        case SCI_GETANCHOR:
        case SCI_GETCARETSTYLE:
        case SCI_GETCHARAT:
        case SCI_GETCODEPAGE:
        case SCI_GETCOLUMN:
        case SCI_GETCURRENTPOS:
        case SCI_GETEOLMODE:
        case SCI_GETLENGTH:
        case SCI_GETLEXER:
        case SCI_GETLINE:
        case SCI_GETLINECOUNT:
        case SCI_GETLINEENDPOSITION:
        case SCI_GETLINESELENDPOSITION:
        case SCI_GETLINESELSTARTPOSITION:
        case SCI_GETMARGINS:
        case SCI_GETMARGINWIDTHN:
        case SCI_GETMAXLINESTATE:
        case SCI_GETREADONLY:
        case SCI_GETSELECTIONNEND:
        case SCI_GETSELECTIONNSTART:
        case SCI_GETSELECTIONSTART:
        case SCI_GETSELECTIONEND:
        case SCI_GETSELTEXT:
        case SCI_GETSTYLEAT:
        case SCI_TARGETFROMSELECTION:
        case SCI_GETTARGETTEXT:
        case SCI_GETTABWIDTH:
        case SCI_GETTEXT:
        case SCI_GETTEXTLENGTH:
        case SCI_GETTEXTRANGE:
        case SCI_GETUSETABS:
        case SCI_GETWRAPMODE:
        case SCI_GRABFOCUS:
        case SCI_GOTOLINE:
        case SCI_GOTOPOS:
        case SCI_INSERTTEXT:
        case SCI_LINEFROMPOSITION:
        case SCI_LINELENGTH:
        case SCI_NEWLINE:
        case SCI_POSITIONFROMLINE:
        case SCI_REPLACESEL:
        case SCI_SELECTIONISRECTANGLE:
        case SCI_SETANCHOR:
        case SCI_SETCARETSTYLE:
        case SCI_SETCURRENTPOS:
        case SCI_SETFIRSTVISIBLELINE:
        case SCI_SETSCROLLWIDTH:
        case SCI_SETTARGETSTART:
        case SCI_SETTARGETEND:
        case SCI_REPLACETARGET:
        case SCI_SELECTALL:
        case SCI_SETSEL:
        case SCI_SETSELECTIONEND:
        case SCI_SETSELECTIONSTART:
        case SCI_SETXCARETPOLICY:
        case SCI_SETXOFFSET:
        case SCI_SETYCARETPOLICY:
        case SCI_SHOWLINES:
        case SCI_SETTEXT:
        case SCI_TEXTHEIGHT:
        case SCI_TEXTWIDTH:
        case SCI_WORDENDPOSITION:
        case SCI_WORDSTARTPOSITION:
        {
            const LRESULT result = static_cast<LRESULT>(
                _editor->SendScintillaNpp(
                    message, static_cast<uptr_t>(wParam),
                    static_cast<sptr_t>(lParam)));
            if (message == SCI_GETSELTEXT && lParam == 0
                && _selectionTextLengthIncludesTerminator) {
                return result + 1;
            }
            return result;
        }
        default:
            if (handled)
                *handled = false;
            return 0;
    }
}
