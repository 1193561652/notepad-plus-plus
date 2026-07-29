#include "NppCommandRegistry.h"

#include <cstring>

const QVector<NppCommandMapping>& nppCommandMappings()
{
    static const QVector<NppCommandMapping> mappings = {
        {"newAction", 41001}, {"openAction", 41002},
        {"closeAction", 41003}, {"closeAllAction", 41004},
        {"closeAllButCurrentAction", 41005}, {"saveAction", 41006},
        {"saveAllAction", 41007}, {"saveAsAction", 41008},
        {"closeAllToLeftAction", 41009}, {"printAction", 41010},
        {"printNowAction", 1001}, {"exitAction", 41011},
        {"loadSessionAction", 41012}, {"saveSessionAction", 41013},
        {"reloadAction", 41014}, {"saveCopyAsAction", 41015},
        {"moveToRecycleBinAction", 41016}, {"renameFileAction", 41017},
        {"closeAllToRightAction", 41018},
        {"openContainingExplorerAction", 41019},
        {"openContainingCmdAction", 41020},
        {"restoreLastClosedFileAction", 41021},
        {"openFolderAsWorkspaceAction", 41022},
        {"openDefaultViewerAction", 41023},
        {"closeAllUnchangedAction", 41024},
        {"containingFolderAsWorkspaceAction", 41025},

        {"cutAction", 42001}, {"copyAction", 42002},
        {"undoAction", 42003}, {"redoAction", 42004},
        {"pasteAction", 42005}, {"deleteSelectionAction", 42006},
        {"selectAllAction", 42007}, {"duplicateLineAction", 42010},
        {"moveLineUpAction", 42014}, {"moveLineDownAction", 42015},
        {"toUpperCaseAction", 42016}, {"toLowerCaseAction", 42017},
        {"startRecordAction", 42018}, {"stopRecordAction", 42019},
        {"beginEndSelectAction", 42020}, {"playMacroAction", 42021},
        {"toggleCommentAction", 42022}, {"streamCommentAction", 42023},
        {"trimTrailingSpaceAction", 42024}, {"saveMacroAction", 42025},
        {"columnEditorAction", 42034}, {"blockCommentAction", 42035},
        {"blockUncommentAction", 42036}, {"columnModeTipAction", 42037},
        {"trimLeadingSpaceAction", 42042}, {"trimBothAction", 42043},
        {"tabToSpaceAction", 42046}, {"characterPanelAction", 42051},
        {"clipboardHistoryAction", 42052},
        {"spaceToTabLeadingAction", 42053}, {"spaceToTabAllAction", 42054},
        {"removeEmptyLinesAction", 42055}, {"removeBlankLinesAction", 42056},
        {"sortLexicographicAscendingAction", 42059},
        {"sortLexicographicDescendingAction", 42060},
        {"sortIntegerAscendingAction", 42061},
        {"sortIntegerDescendingAction", 42062},
        {"sortDecimalCommaAscendingAction", 42063},
        {"sortDecimalCommaDescendingAction", 42064},
        {"sortDecimalAscendingAction", 42065},
        {"sortDecimalDescendingAction", 42066},
        {"properCaseAction", 42067}, {"sentenceCaseAction", 42069},
        {"invertCaseAction", 42071}, {"searchOnInternetAction", 42075},
        {"sortRandomAction", 42078},
        {"sortAscendingIgnoreCaseAction", 42080},
        {"sortDescendingIgnoreCaseAction", 42081},
        {"reverseLineOrderAction", 42083},
        {"insertDateTimeShortAction", 42084},
        {"insertDateTimeLongAction", 42085},
        {"insertDateTimeCustomAction", 42086},
        {"functionCompletionAction", 50000},
        {"wordCompletionAction", 50001},
        {"functionParametersHintAction", 50002},

        {"findAction", 43001}, {"findNextAction", 43002},
        {"replaceAction", 43003}, {"goToLineAction", 43004},
        {"toggleBookmarkAction", 43005}, {"nextBookmarkAction", 43006},
        {"prevBookmarkAction", 43007}, {"clearBookmarksAction", 43008},
        {"goToMatchingBraceAction", 43009},
        {"findPreviousAction", 43010}, {"findInFilesAction", 43013},
        {"clearAllMarksAction", 43032}, {"jumpUpMarkedAction", 43038},
        {"jumpDownMarkedAction", 43044}, {"markDialogAction", 43054},

        {"postItAction", 44009}, {"collapseAllAction", 44010},
        {"showAllCharactersAction", 44019}, {"showIndentAction", 44020},
        {"wordWrapAction", 44022}, {"zoomInAction", 44023},
        {"zoomOutAction", 44024}, {"showWhitespaceAction", 44025},
        {"showEolAction", 44026}, {"uncollapseAllAction", 44029},
        {"foldCurrentLevelAction", 44030},
        {"unfoldCurrentLevelAction", 44031}, {"fullScreenAction", 44032},
        {"zoomRestoreAction", 44033}, {"alwaysOnTopAction", 44034},
        {"documentListAction", 44070}, {"docMapAction", 44080},
        {"projectPanelsAction", 44081}, {"funcListAction", 44084},
        {"fileBrowserAction", 44085},
        {"moveToOtherAction", 10001}, {"cloneToOtherAction", 10002},

        {"eolWindowsAction", 45001}, {"eolUnixAction", 45002},
        {"eolMacAction", 45003},
        {"styleConfiguratorAction", 46001},
        {"userDefinedLanguageDialogAction", 46250},

        {"homeAction", 47001}, {"onlineDocsAction", 47003},
        {"cmdArgsAction", 47010}, {"debugInfoAction", 47012},
        {"aboutAction", 47000},

        {"importPluginsAction", 48005}, {"importStyleThemesAction", 48006},
        {"shortcutMapperAction", 48009}, {"preferencesAction", 48011},
        {"editContextMenuAction", 48018},
        {"md5GenerateAction", 48501}, {"md5FromFilesAction", 48502},
        {"md5ToClipboardAction", 48503}, {"sha256GenerateAction", 48504},
        {"sha256FromFilesAction", 48505}, {"sha256ToClipboardAction", 48506},
        {"runDialogAction", 49000}, {"windowsDialogAction", 11001},
        {"sortByNameAscAction", 11002}, {"sortByNameDescAction", 11003},
        {"copyNamesAction", 11060}, {"copyPathsAction", 11061}
    };
    return mappings;
}

int nppCommandIdForObjectName(const char* objectName)
{
    if (!objectName)
        return 0;
    for (const NppCommandMapping& mapping : nppCommandMappings()) {
        if (std::strcmp(mapping.objectName, objectName) == 0)
            return mapping.commandId;
    }
    return 0;
}
