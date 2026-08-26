#include "UiResourceLoader.h"

#include <QImage>

namespace NppUiResources {

QPixmap bitmap(const QString& resourcePath, BitmapMode mode,
               const QSize& logicalSize)
{
    QImage image(resourcePath);
    if (image.isNull())
        return QPixmap();

    if (mode != BitmapMode::Opaque) {
        image = image.convertToFormat(QImage::Format_ARGB32);
        const QRgb mask = mode == BitmapMode::MaskGray192
            ? qRgb(192, 192, 192) : image.pixel(0, 0);
        const int red = qRed(mask);
        const int green = qGreen(mask);
        const int blue = qBlue(mask);
        for (int y = 0; y < image.height(); ++y) {
            QRgb* scanLine = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x) {
                const QRgb pixel = scanLine[x];
                if (qRed(pixel) == red && qGreen(pixel) == green &&
                    qBlue(pixel) == blue) {
                    scanLine[x] = qRgba(0, 0, 0, 0);
                }
            }
        }
    }

    QPixmap pixmap = QPixmap::fromImage(image);
    if (logicalSize.isValid() && pixmap.size() != logicalSize) {
        pixmap = pixmap.scaled(logicalSize, Qt::IgnoreAspectRatio,
                               Qt::SmoothTransformation);
    }
    return pixmap;
}

QIcon bitmapIcon(const QString& resourcePath, BitmapMode mode,
                 const QSize& logicalSize)
{
    return QIcon(bitmap(resourcePath, mode, logicalSize));
}

QIcon panelIcon(PanelIcon panel, bool darkMode, bool alternateSet)
{
    const char* baseName = nullptr;
    switch (panel) {
    case PanelIcon::DocumentMap: baseName = "docMap"; break;
    case PanelIcon::DocumentList: baseName = "docList"; break;
    case PanelIcon::FunctionList: baseName = "functionList"; break;
    case PanelIcon::Project: baseName = "projectPanel"; break;
    case PanelIcon::FileBrowser: baseName = "fileBrowser"; break;
    case PanelIcon::Clipboard: baseName = "clipboardPanel"; break;
    case PanelIcon::Character: baseName = "asciiPanel"; break;
    case PanelIcon::FindResult:
        return QIcon(QStringLiteral(":/icons/findResult.ico"));
    }
    const QString name = QString::fromLatin1(baseName);
    if (darkMode)
        return QIcon(QStringLiteral(":/icons/darkMode/panels/%1.ico").arg(name));
    return QIcon(QStringLiteral(":/icons/%1%2.ico")
                     .arg(name, alternateSet ? QStringLiteral("2") : QString()));
}

QIcon documentIcon(DocumentState state, bool darkMode, bool alternateSet)
{
    QString name;
    switch (state) {
    case DocumentState::Saved: name = alternateSet ? "saved_alt.ico" : "saved.ico"; break;
    case DocumentState::Modified: name = alternateSet ? "unsaved_alt.ico" : "unsaved.ico"; break;
    case DocumentState::ReadOnly: name = alternateSet ? "readonly_alt.ico" : "readonly.ico"; break;
    case DocumentState::Monitoring: name = "monitoring.ico"; break;
    }
    QString prefix = QStringLiteral(":/icons/");
    if (!alternateSet && darkMode)
        prefix += QStringLiteral("darkMode/tabbar/");
    return QIcon(prefix + name);
}

QPixmap tabClosePixmap(TabCloseState state, bool darkMode,
                       const QSize& logicalSize)
{
    QString name = QStringLiteral("closeTabButton");
    switch (state) {
    case TabCloseState::Normal: break;
    case TabCloseState::Hover: name += QStringLiteral("_hover"); break;
    case TabCloseState::Pressed: name += QStringLiteral("_push"); break;
    case TabCloseState::Inactive: name += QStringLiteral("_inact"); break;
    }
    const QString path = darkMode
        ? QStringLiteral(":/icons/darkMode/tabbar/%1.bmp").arg(name)
        : QStringLiteral(":/icons/%1.bmp").arg(name);
    return bitmap(path,
                  BitmapMode::Opaque, logicalSize);
}

QIcon aboutIcon(bool darkMode)
{
    return QIcon(darkMode
        ? QStringLiteral(":/icons/darkMode/about/chameleon.ico")
        : QStringLiteral(":/icons/chameleon.ico"));
}

} // namespace NppUiResources
