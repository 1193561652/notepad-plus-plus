#ifndef UIRESOURCELOADER_H
#define UIRESOURCELOADER_H

#include <QIcon>
#include <QPixmap>
#include <QSize>
#include <QString>

namespace NppUiResources {

enum class BitmapMode {
    Opaque,
    TopLeftTransparent,
    MaskGray192
};

enum class PanelIcon {
    DocumentMap,
    DocumentList,
    FunctionList,
    Project,
    FileBrowser,
    Clipboard,
    Character,
    FindResult
};

enum class DocumentState {
    Saved,
    Modified,
    ReadOnly,
    Monitoring
};

enum class TabCloseState {
    Normal,
    Hover,
    Pressed,
    Inactive
};

QPixmap bitmap(const QString& resourcePath,
               BitmapMode mode = BitmapMode::TopLeftTransparent,
               const QSize& logicalSize = QSize());
QIcon bitmapIcon(const QString& resourcePath,
                 BitmapMode mode = BitmapMode::TopLeftTransparent,
                 const QSize& logicalSize = QSize());
QIcon panelIcon(PanelIcon panel, bool darkMode, bool alternateSet);
QIcon documentIcon(DocumentState state, bool darkMode, bool alternateSet);
QPixmap tabClosePixmap(TabCloseState state, bool darkMode,
                       const QSize& logicalSize = QSize(11, 11));
QIcon aboutIcon(bool darkMode);

} // namespace NppUiResources

#endif
