#pragma once
#include <QMimeData>
// Names match the host's ScintillaQt::AddRectangularToMime/IsRectangularInMime.
namespace QtPlugin {
inline void markRectangular(QMimeData* mime) {
#if defined(Q_OS_WIN)
    mime->setData("MSDEVColumnSelect",{});
    mime->setData("application/x-qt-windows-mime;value=\"MSDEVColumnSelect\"",{});
#elif defined(Q_OS_MAC)
    mime->setData("text/x-scintilla.utf16-plain-text.rectangular",mime->text().toUtf8());
#else
    mime->setData("text/x-rectangular-marker",{});
#endif
}
inline bool isRectangular(const QMimeData* mime) {
    return mime && (mime->hasFormat("MSDEVColumnSelect") ||
        mime->hasFormat("application/x-qt-windows-mime;value=\"MSDEVColumnSelect\"") ||
        mime->hasFormat("text/x-scintilla.utf16-plain-text.rectangular") ||
        mime->hasFormat("text/x-rectangular-marker"));
}
}
