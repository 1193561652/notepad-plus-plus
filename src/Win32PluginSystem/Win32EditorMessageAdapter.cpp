#include "Win32PluginSystem/Win32EditorMessageAdapter.h"

#include <QDebug>

#include "ScintillaComponent/ScintillaEditView.h"

namespace {

struct LegacyCharacterRange
{
    LONG cpMin;
    LONG cpMax;
};

struct LegacyTextRange
{
    LegacyCharacterRange chrg;
    char* text;
};

struct LegacyTextToFind
{
    LegacyCharacterRange chrg;
    const char* text;
    LegacyCharacterRange chrgText;
};

struct LegacyRangeToFormat
{
    Sci_SurfaceID hdc;
    Sci_SurfaceID hdcTarget;
    Sci_Rectangle rc;
    Sci_Rectangle rcPage;
    LegacyCharacterRange chrg;
};

} // namespace

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
        case SCI_GETDIRECTFUNCTION:
            return reinterpret_cast<LRESULT>(
                &Win32EditorMessageAdapter::directFunction);
        case SCI_GETDIRECTPOINTER:
            return reinterpret_cast<LRESULT>(this);
        case SCI_BEGINUNDOACTION:
        case SCI_ANNOTATIONCLEARALL:
        case SCI_ANNOTATIONGETSTYLES:
        case SCI_ANNOTATIONGETTEXT:
        case SCI_ANNOTATIONSETSTYLES:
        case SCI_ANNOTATIONSETTEXT:
        case SCI_ANNOTATIONSETVISIBLE:
        case SCI_ADDTABSTOP:
        case SCI_ADDTEXT:
        case SCI_APPENDTEXT:
        case SCI_BRACEHIGHLIGHT:
        case SCI_CALLTIPCANCEL:
        case SCI_CALLTIPSHOW:
        case SCI_CLEARALL:
        case SCI_CLEARSELECTIONS:
        case SCI_CLEARTABSTOPS:
        case SCI_EMPTYUNDOBUFFER:
        case SCI_ENDUNDOACTION:
        case SCI_ENSUREVISIBLE:
        case SCI_FINDCOLUMN:
        case SCI_GETANCHOR:
        case SCI_GETAUTOMATICFOLD:
        case SCI_GETBACKSPACEUNINDENTS:
        case SCI_GETCARETSTYLE:
        case SCI_GETCHARAT:
        case SCI_GETCODEPAGE:
        case SCI_GETCOLUMN:
        case SCI_GETCURRENTPOS:
        case SCI_GETEOLMODE:
        case SCI_GETFIRSTVISIBLELINE:
        case SCI_GETINDENT:
        case SCI_GETLENGTH:
        case SCI_GETLEXER:
        case SCI_GETLINE:
        case SCI_GETLINECOUNT:
        case SCI_GETLINEENDPOSITION:
        case SCI_GETLINEINDENTATION:
        case SCI_GETLINESELENDPOSITION:
        case SCI_GETLINESELSTARTPOSITION:
        case SCI_GETMARGINS:
        case SCI_GETMARGINWIDTHN:
        case SCI_GETMAXLINESTATE:
        case SCI_GETMODIFY:
        case SCI_GETNEXTTABSTOP:
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
        case SCI_GETTABINDENTS:
        case SCI_GETTEXT:
        case SCI_GETTEXTLENGTH:
        case SCI_GETTEXTRANGE:
        case SCI_GETUSETABS:
        case SCI_GETWRAPMODE:
        case SCI_GRABFOCUS:
        case SCI_GOTOLINE:
        case SCI_GOTOPOS:
        case SCI_INSERTTEXT:
        case SCI_INDICSETALPHA:
        case SCI_INDICSETOUTLINEALPHA:
        case SCI_INDICSETSTYLE:
        case SCI_LINEFROMPOSITION:
        case SCI_LINELENGTH:
        case SCI_LINESONSCREEN:
        case SCI_NEWLINE:
        case SCI_POSITIONFROMLINE:
        case SCI_POSITIONAFTER:
        case SCI_REPLACESEL:
        case SCI_SELECTIONISRECTANGLE:
        case SCI_SETANCHOR:
        case SCI_SETAUTOMATICFOLD:
        case SCI_SETBACKSPACEUNINDENTS:
        case SCI_SETCARETSTYLE:
        case SCI_SETCURRENTPOS:
        case SCI_SETINDENT:
        case SCI_SETEOLMODE:
        case SCI_SETFIRSTVISIBLELINE:
        case SCI_SETSCROLLWIDTH:
        case SCI_SETTARGETSTART:
        case SCI_SETTARGETEND:
        case SCI_SETTABWIDTH:
        case SCI_SETTABINDENTS:
        case SCI_SETUSETABS:
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
            if (qEnvironmentVariableIsSet(
                    "NPP_QT_TEST_PLUGIN_MESSAGE_TRACE")) {
                qWarning() << "Unhandled Win32 plugin SCI message"
                           << message << "wParam" << wParam;
            }
            if (handled)
                *handled = false;
            return 0;
    }
}

sptr_t Win32EditorMessageAdapter::directFunction(
    sptr_t pointer, unsigned int message, uptr_t wParam, sptr_t lParam)
{
    const Win32EditorMessageAdapter* adapter =
        reinterpret_cast<const Win32EditorMessageAdapter*>(pointer);
    return adapter
        ? adapter->sendLegacyDirectMessage(message, wParam, lParam)
        : 0;
}

sptr_t Win32EditorMessageAdapter::sendLegacyDirectMessage(
    unsigned int message, uptr_t wParam, sptr_t lParam) const
{
    if (_legacyTextRangeAbi
        && (message == SCI_GETTEXTRANGE || message == SCI_GETSTYLEDTEXT)
        && lParam) {
        LegacyTextRange* legacy = reinterpret_cast<LegacyTextRange*>(lParam);
        Sci_TextRange translated{
            {static_cast<Sci_PositionCR>(legacy->chrg.cpMin),
             static_cast<Sci_PositionCR>(legacy->chrg.cpMax)},
            legacy->text
        };
        return _editor->SendScintillaNpp(
            message, wParam, reinterpret_cast<sptr_t>(&translated));
    }
    if (_legacyTextRangeAbi && message == SCI_FINDTEXT && lParam) {
        LegacyTextToFind* legacy = reinterpret_cast<LegacyTextToFind*>(lParam);
        Sci_TextToFind translated{
            {static_cast<Sci_PositionCR>(legacy->chrg.cpMin),
             static_cast<Sci_PositionCR>(legacy->chrg.cpMax)},
            legacy->text,
            {0, 0}
        };
        const sptr_t result = _editor->SendScintillaNpp(
            message, wParam, reinterpret_cast<sptr_t>(&translated));
        legacy->chrgText.cpMin = static_cast<LONG>(translated.chrgText.cpMin);
        legacy->chrgText.cpMax = static_cast<LONG>(translated.chrgText.cpMax);
        return result;
    }
    if (_legacyTextRangeAbi && message == SCI_FORMATRANGE && lParam) {
        LegacyRangeToFormat* legacy =
            reinterpret_cast<LegacyRangeToFormat*>(lParam);
        Sci_RangeToFormat translated{
            legacy->hdc, legacy->hdcTarget, legacy->rc, legacy->rcPage,
            {static_cast<Sci_PositionCR>(legacy->chrg.cpMin),
             static_cast<Sci_PositionCR>(legacy->chrg.cpMax)}
        };
        return _editor->SendScintillaNpp(
            message, wParam, reinterpret_cast<sptr_t>(&translated));
    }
    return _editor->SendScintillaNpp(message, wParam, lParam);
}
