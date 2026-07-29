#include <QDomDocument>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QString>

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

QString readFile(const QString& path)
{
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "test source file is not readable");
    return QString::fromUtf8(file.readAll());
}

} // namespace

int main()
{
    const QString mainWindowSource =
        readFile(QStringLiteral(NPP_MAINWINDOW_SOURCE));
    const QString findDialogSource =
        readFile(QStringLiteral(NPP_FIND_DIALOG_SOURCE));
    const QString preferencesSource =
        readFile(QStringLiteral(NPP_PREFERENCES_SOURCE));

    require(mainWindowSource.count(QStringLiteral("new FindReplaceDlg(this)")) == 2,
            "FindReplaceDlg must only be created at startup and by the shared helper");
    require(mainWindowSource.count(QStringLiteral("ensureFindReplaceDialog()")) >= 8,
            "all FindReplaceDlg entry points must use the shared localization helper");

    QFile languageFile(QStringLiteral(NPP_FIND_LANGUAGE_SOURCE));
    QDomDocument languageDocument;
    require(languageFile.open(QIODevice::ReadOnly)
                && languageDocument.setContent(&languageFile),
            "Find dialog language XML is invalid");

    QSet<QString> translatedNames;
    QSet<QString> preferenceTranslatedNames;
    const QDomNodeList dialogs = languageDocument.elementsByTagName("Dialog");
    for (int i = 0; i < dialogs.count(); ++i) {
        const QDomElement dialog = dialogs.at(i).toElement();
        QSet<QString>* target = nullptr;
        if (dialog.attribute("name") == QStringLiteral("FindReplace"))
            target = &translatedNames;
        else if (dialog.attribute("name") == QStringLiteral("Preferences"))
            target = &preferenceTranslatedNames;
        if (!target)
            continue;
        for (QDomElement item = dialog.firstChildElement("Item");
             !item.isNull(); item = item.nextSiblingElement("Item")) {
            target->insert(item.attribute("objectName"));
        }
    }

    const QStringList requiredNames = {
        "tabBar_0", "tabBar_1", "tabBar_2", "tabBar_3", "tabBar_4",
        "lblFindWhat", "lblReplaceWith", "lblFilters", "lblDirectory",
        "chkInSel", "browseDirBtn",
        "chkBackwardDir", "chkWholeWord", "chkMatchCase", "chkWrapAround",
        "chkMatchCase2", "chkWholeWord2", "chkWrapAround2", "chkInSel2",
        "chkMatchCase3", "chkWholeWord3", "chkFollowDoc", "chkRecursive",
        "chkInHiddenDir",
        "chkProjectPanel1", "chkProjectPanel2", "chkProjectPanel3",
        "chkBookmarkLine", "chkPurgeMarks", "chkMatchCase4", "chkWholeWord4",
        "chkInSel4",
        "btnFindNext", "btnCount", "btnFindAllOpened", "btnFindAllCur",
        "btnClose", "btnFindNext2", "btnReplace", "btnReplaceAll",
        "btnReplAllOpened", "btnClose2", "btnFindAllFif",
        "btnReplaceInFiles", "btnClose3", "btnFindAllFip",
        "btnReplaceInProjects", "btnClose4", "btnMarkAll", "btnClearMarks",
        "btnCopyMarked", "btnClose5",
        "grpSearchMode", "rbModeNormal", "rbModeExtended", "rbModeRegex",
        "chkDotMatchNewline", "grpTransparency", "rbTransOnLostFocus",
        "rbTransAlways"
    };

    for (const QString& name : requiredNames) {
        require(translatedNames.contains(name),
                qPrintable(QStringLiteral("missing FindReplace language item: %1")
                               .arg(name)));
        if (!name.startsWith(QStringLiteral("tabBar_"))) {
            require(findDialogSource.contains(
                        QStringLiteral("setObjectName(\"%1\")").arg(name)),
                    qPrintable(QStringLiteral("missing FindReplace objectName: %1")
                                   .arg(name)));
        }
    }

    const QStringList requiredPreferenceNames = {
        "grpAutoInsert", "chkPairParentheses", "chkPairBrackets",
        "chkPairCurly", "chkPairQuotes", "chkPairDoubleQuotes",
        "chkPairTags", "lblEdgeColumn", "chkBackupCustomDir",
        "btnBackupDirBrowse", "lblSupportedExtensions",
        "lblRegisteredExtensions", "lblFileAssociationAdmin",
        "lblFileAssociationPlatform", "grpTagMatching", "chkTagMatch",
        "chkTagAttributes"
    };
    for (const QString& name : requiredPreferenceNames) {
        require(preferenceTranslatedNames.contains(name),
                qPrintable(QStringLiteral("missing Preferences language item: %1")
                               .arg(name)));
        require(preferencesSource.contains(
                    QStringLiteral("setObjectName(\"%1\")").arg(name)),
                qPrintable(QStringLiteral("missing Preferences objectName: %1")
                                   .arg(name)));
    }

    const QStringList preferenceLines = preferencesSource.split(QLatin1Char('\n'));
    const QRegularExpression localizableConstructor(
        QStringLiteral(
            "new\\s+(QLabel|QGroupBox|QCheckBox|QRadioButton|"
            "QPushButton|QToolButton)\\s*\\(\\s*tr\\("));
    for (int i = 0; i < preferenceLines.size(); ++i) {
        if (!localizableConstructor.match(preferenceLines.at(i)).hasMatch())
            continue;
        const int lastLine = qMin(i + 6, preferenceLines.size() - 1);
        const QString nearby =
            preferenceLines.mid(i, lastLine - i + 1).join(QLatin1Char('\n'));
        require(nearby.contains(QStringLiteral("setObjectName(")),
                qPrintable(QStringLiteral(
                    "localizable Preferences control lacks objectName near line %1")
                               .arg(i + 1)));
    }

    return 0;
}
