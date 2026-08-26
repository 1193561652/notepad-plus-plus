#include "WinControls/UiResourceLoader.h"

#include <QGuiApplication>
#include <QImage>
#include <iostream>

namespace {
bool expect(bool condition, const char* message)
{
    if (!condition)
        std::cerr << "FAILED: " << message << '\n';
    return condition;
}

bool containsTransparentPixel(const QPixmap& pixmap)
{
    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) == 0)
                return true;
        }
    }
    return false;
}
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    bool ok = true;

    const QPixmap treeIcon = NppUiResources::bitmap(
        QStringLiteral(":/icons/project_folder_close.bmp"),
        NppUiResources::BitmapMode::MaskGray192);
    ok &= expect(!treeIcon.isNull(), "original tree bitmap must load");
    ok &= expect(containsTransparentPixel(treeIcon),
                 "tree bitmap gray mask must become transparent");

    const QPixmap closeIcon = NppUiResources::tabClosePixmap(
        NppUiResources::TabCloseState::Pressed, true);
    ok &= expect(closeIcon.size() == QSize(11, 11),
                 "tab close bitmap must use the original logical size");
    ok &= expect(!containsTransparentPixel(closeIcon),
                 "tab close bitmap must remain opaque like upstream SRCCOPY");
    ok &= expect(!NppUiResources::tabClosePixmap(
                     NppUiResources::TabCloseState::Normal, false).isNull(),
                 "light tab close bitmap must load from the original root path");

    ok &= expect(!NppUiResources::panelIcon(
                     NppUiResources::PanelIcon::FileBrowser, true, false).isNull(),
                 "dark panel icon must load");
    ok &= expect(!NppUiResources::panelIcon(
                     NppUiResources::PanelIcon::Project, false, true).isNull(),
                 "alternate panel icon must load");
    ok &= expect(!NppUiResources::documentIcon(
                     NppUiResources::DocumentState::Modified, true, false).isNull(),
                 "dark document state icon must load");
    ok &= expect(!NppUiResources::aboutIcon(true).isNull(),
                 "dark About icon must load");

    return ok ? 0 : 1;
}
