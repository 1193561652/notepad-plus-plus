#include "WinControls/ToolBar/ToolbarIconTheme.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QTemporaryDir>

#include <iostream>

namespace
{
bool expect(bool condition, const char* message)
{
    if (!condition)
        std::cerr << "FAILED: " << message << '\n';
    return condition;
}

bool writeFile(const QString& path, const QByteArray& data = {})
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    bool ok = true;
    QTemporaryDir temporary;
    ok &= expect(temporary.isValid(), "temporary directory must be available");
    ok &= expect(!QIcon(QStringLiteral(":/icons/new_off.ico")).isNull(),
                 "the original Fluent resource set must be embedded");
    ok &= expect(!QIcon(QStringLiteral(
                     ":/icons/filledFluentUI/new_off.ico")).isNull(),
                 "the original filled Fluent resource set must be embedded");
    ok &= expect(!QIcon(QStringLiteral(
                     ":/icons/darkMode/toolbar/FluentUI/new_off.ico")).isNull(),
                 "the original dark Fluent resource set must be embedded");
    ok &= expect(!QIcon(QStringLiteral(
                     ":/icons/darkMode/toolbar/filledFluentUI/new_off.ico"))
                     .isNull(),
                 "the original dark filled resource set must be embedded");

    QDir root(temporary.path());
    root.mkpath(QStringLiteral("toolbarIcons/custom"));
    ok &= expect(writeFile(root.filePath(QStringLiteral("toolbarIcons.xml")),
        "<NotepadPlus><ToolBarIcons icoFolderName=\"custom\"/></NotepadPlus>"),
        "toolbarIcons.xml must be writable");
    const QString customIcon =
        root.filePath(QStringLiteral("toolbarIcons/custom/new.ico"));
    ok &= expect(QFile::copy(QStringLiteral(NPP_TEST_ICON_SOURCE), customIcon),
                 "custom icon must be copied into the theme");

    ToolbarIconTheme theme =
        ToolbarIconTheme::fromUserDirectory(temporary.path());
    ok &= expect(theme.isConfigured(), "valid XML must configure a theme");
    ok &= expect(theme.iconDirectory().endsWith(
        QStringLiteral("toolbarIcons/custom")),
        "configured folder name must be respected");
    ok &= expect(!theme.iconPath(QStringLiteral("new")).isEmpty(),
                 "existing fixed-name icon must be found");
    ok &= expect(!theme.icon(QStringLiteral("new")).isNull(),
                 "existing custom icon must be loadable");
    ok &= expect(theme.hasIcon(QStringLiteral("new")),
                 "an existing custom override must be reported");
    ok &= expect(theme.iconPath(QStringLiteral("open")).isEmpty(),
                 "missing custom icons must fall back");

    root.mkpath(QStringLiteral("toolbarIcons/default"));
    ok &= expect(writeFile(root.filePath(QStringLiteral("toolbarIcons.xml")),
        "<NotepadPlus><ToolBarIcons icoFolderName=\"\"/></NotepadPlus>"),
        "default toolbar XML must be writable");
    theme = ToolbarIconTheme::fromUserDirectory(temporary.path());
    ok &= expect(theme.iconDirectory().endsWith(
        QStringLiteral("toolbarIcons/default")),
        "empty folder name must select default");

    ok &= expect(writeFile(root.filePath(QStringLiteral("toolbarIcons.xml")),
        "<NotepadPlus><ToolBarIcons icoFolderName=\"../outside\"/></NotepadPlus>"),
        "invalid toolbar XML must be writable");
    theme = ToolbarIconTheme::fromUserDirectory(temporary.path());
    ok &= expect(!theme.isConfigured(),
                 "folder traversal must not escape the configuration directory");
    return ok ? 0 : 1;
}
