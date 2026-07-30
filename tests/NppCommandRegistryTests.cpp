#include "NppCommandRegistry.h"

#include <QFile>
#include <QSet>
#include <QString>

#include <iostream>

namespace
{
bool expect(bool condition, const char* message)
{
    if (!condition)
        std::cerr << "FAILED: " << message << '\n';
    return condition;
}
}

int main()
{
    bool ok = true;
    const QVector<NppCommandMapping>& mappings = nppCommandMappings();
    ok &= expect(mappings.size() >= 150, "registry must cover implemented commands");

    QSet<QString> names;
    QSet<int> ids;
    QFile source(QStringLiteral(NPP_MAINWINDOW_SOURCE));
    ok &= expect(source.open(QFile::ReadOnly), "MainWindow.cpp must be readable");
    const QString mainWindowSource = QString::fromUtf8(source.readAll());

    for (const NppCommandMapping& mapping : mappings) {
        const QString name = QString::fromLatin1(mapping.objectName);
        ok &= expect(!name.isEmpty(), "command object name must not be empty");
        ok &= expect(mapping.commandId > 0, "command ID must be positive");
        ok &= expect(!names.contains(name), "command object names must be unique");
        ok &= expect(!ids.contains(mapping.commandId), "command IDs must be unique");
        ok &= expect(mainWindowSource.contains(name),
                     "mapped action must exist in MainWindow.cpp");
        names.insert(name);
        ids.insert(mapping.commandId);
    }

    ok &= expect(nppCommandIdForObjectName("wordWrapAction") == 44022,
                 "Word Wrap must use IDM_VIEW_WRAP");
    ok &= expect(nppCommandIdForObjectName("zoomRestoreAction") == 44033,
                 "Restore Zoom must use IDM_VIEW_ZOOMRESTORE");
    ok &= expect(nppCommandIdForObjectName("reverseLineOrderAction") == 42083,
                 "Reverse Line Order must use its original command ID");
    ok &= expect(nppCommandIdForObjectName("functionCompletionAction") == 50000,
                 "Function Completion must retain its special command ID");
    ok &= expect(nppCommandIdForObjectName("splitLinesAction") == 42012,
                 "Split Lines must retain IDM_EDIT_SPLIT_LINES");
    ok &= expect(nppCommandIdForObjectName("selectToMatchingBraceAction") == 43053,
                 "brace selection must retain its original command ID");
    ok &= expect(nppCommandIdForObjectName("activateTab9Action") == 44094,
                 "tab activation must retain its original command ID");
    ok &= expect(nppCommandIdForObjectName("missingAction") == 0,
                 "unknown actions must not resolve to a command");
    return ok ? 0 : 1;
}
