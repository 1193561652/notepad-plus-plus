// localization.cpp - NativeLangSpeaker 的 Qt 平台实现
// 对应原版 localization.cpp

#include "localization.h"
#include "NppCommandRegistry.h"
#include "TinyXml/tinyxml.h"

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QAbstractButton>
#include <QDockWidget>
#include <QToolBar>
#include <QListWidget>
#include <QTabBar>
#include <QComboBox>
#include <QSpinBox>
#include <QTableWidget>
#include <QTreeWidget>
#include <QFile>
#include <QVariant>

// ── 内部辅助：宽字符属性读取 ─────────────────────────────────────────────────

static inline QString txAttr(const TiXmlElement* el, const wchar_t* name)
{
    const wchar_t* v = el ? el->Attribute(name) : nullptr;
    return v ? QString::fromWCharArray(v) : QString();
}

static QString normalizedNativeText(QString text)
{
    text.remove(QLatin1Char('&'));
    text.replace(QChar(0x2026), QStringLiteral("..."));
    return text.simplified();
}

static QString menuObjectName(const QString& originalId)
{
    static const QMap<QString, QString> ids = {
        {QStringLiteral("file"), QStringLiteral("fileMenu")},
        {QStringLiteral("edit"), QStringLiteral("editMenu")},
        {QStringLiteral("search"), QStringLiteral("searchMenu")},
        {QStringLiteral("view"), QStringLiteral("viewMenu")},
        {QStringLiteral("encoding"), QStringLiteral("encodingMenu")},
        {QStringLiteral("language"), QStringLiteral("languageMenu")},
        {QStringLiteral("settings"), QStringLiteral("settingsMenu")},
        {QStringLiteral("tools"), QStringLiteral("toolsMenu")},
        {QStringLiteral("macro"), QStringLiteral("macroMenu")},
        {QStringLiteral("run"), QStringLiteral("runMenu")},
        {QStringLiteral("Plugins"), QStringLiteral("pluginsMenu")},
        {QStringLiteral("Window"), QStringLiteral("windowMenu")},
        {QStringLiteral("file-openFolder"), QStringLiteral("openContainingFolderMenu")},
        {QStringLiteral("file-closeMore"), QStringLiteral("closeMultipleMenu")},
        {QStringLiteral("file-recentFiles"), QStringLiteral("recentFilesMenu")},
        {QStringLiteral("edit-insert"), QStringLiteral("insertMenu")},
        {QStringLiteral("edit-convertCaseTo"), QStringLiteral("caseMenu")},
        {QStringLiteral("edit-lineOperations"), QStringLiteral("lineOpsMenu")},
        {QStringLiteral("edit-comment"), QStringLiteral("commentMenu")},
        {QStringLiteral("edit-autoCompletion"), QStringLiteral("autoCompletionMenu")},
        {QStringLiteral("edit-eolConversion"), QStringLiteral("eolEditMenu")},
        {QStringLiteral("edit-blankOperations"), QStringLiteral("blankOperationsMenu")},
        {QStringLiteral("search-bookmark"), QStringLiteral("bookmarkMenu")},
        {QStringLiteral("view-showSymbol"), QStringLiteral("showSymMenu")},
        {QStringLiteral("view-zoom"), QStringLiteral("zoomMenu")},
        {QStringLiteral("view-moveCloneDocument"), QStringLiteral("moveCloneMenu")},
        {QStringLiteral("language-userDefinedLanguage"), QStringLiteral("userDefinedLanguageMenu")},
        {QStringLiteral("settings-import"), QStringLiteral("importMenu")},
        {QStringLiteral("tools-md5"), QStringLiteral("md5Menu")},
        {QStringLiteral("tools-sha256"), QStringLiteral("sha256Menu")},
        {QStringLiteral("encoding-characterSets"), QStringLiteral("charsetsMenu")},
        {QStringLiteral("encoding-arabic"), QStringLiteral("arabicMenu")},
        {QStringLiteral("encoding-baltic"), QStringLiteral("balticMenu")},
        {QStringLiteral("encoding-celtic"), QStringLiteral("celticMenu")},
        {QStringLiteral("encoding-centralEuropean"), QStringLiteral("centralEurMenu")},
        {QStringLiteral("encoding-chinese"), QStringLiteral("chineseMenu")},
        {QStringLiteral("encoding-cyrillic"), QStringLiteral("cyrillicMenu")},
        {QStringLiteral("encoding-greek"), QStringLiteral("greekMenu")},
        {QStringLiteral("encoding-hebrew"), QStringLiteral("hebrewMenu")},
        {QStringLiteral("encoding-japanese"), QStringLiteral("japaneseMenu")},
        {QStringLiteral("encoding-korean"), QStringLiteral("koreanMenu")},
        {QStringLiteral("encoding-northEuropean"), QStringLiteral("nordicMenu")},
        {QStringLiteral("encoding-thai"), QStringLiteral("thaiMenu")},
        {QStringLiteral("encoding-turkish"), QStringLiteral("turkishMenu")},
        {QStringLiteral("encoding-vietnamese"), QStringLiteral("vietnameseMenu")},
        {QStringLiteral("encoding-westernEuropean"), QStringLiteral("westernEurMenu")}
    };
    return ids.value(originalId);
}

static QStringList findDialogObjectNames(int originalId)
{
    static const QMap<int, QStringList> ids = {
        {1, {QStringLiteral("btnFindNext"), QStringLiteral("btnFindNext2")}},
        {2, {QStringLiteral("btnClose"), QStringLiteral("btnClose2"),
             QStringLiteral("btnClose3"), QStringLiteral("btnClose4"),
             QStringLiteral("btnClose5")}},
        {1603, {QStringLiteral("chkWholeWord"), QStringLiteral("chkWholeWord2"),
                QStringLiteral("chkWholeWord3"), QStringLiteral("chkWholeWord4")}},
        {1604, {QStringLiteral("chkMatchCase"), QStringLiteral("chkMatchCase2"),
                QStringLiteral("chkMatchCase3"), QStringLiteral("chkMatchCase4")}},
        {1605, {QStringLiteral("rbModeRegex")}},
        {1606, {QStringLiteral("chkWrapAround"), QStringLiteral("chkWrapAround2")}},
        {1608, {QStringLiteral("btnReplace")}},
        {1609, {QStringLiteral("btnReplaceAll")}},
        {1611, {QStringLiteral("lblReplaceWith")}},
        {1614, {QStringLiteral("btnCount")}},
        {1615, {QStringLiteral("btnMarkAll")}},
        {1616, {QStringLiteral("chkBookmarkLine")}},
        {1618, {QStringLiteral("chkPurgeMarks")}},
        {1620, {QStringLiteral("lblFindWhat")}},
        {1624, {QStringLiteral("grpSearchMode")}},
        {1625, {QStringLiteral("rbModeNormal")}},
        {1626, {QStringLiteral("rbModeExtended")}},
        {1632, {QStringLiteral("chkInSel"), QStringLiteral("chkInSel2"),
                QStringLiteral("chkInSel4")}},
        {1633, {QStringLiteral("btnClearMarks")}},
        {1635, {QStringLiteral("btnReplAllOpened")}},
        {1636, {QStringLiteral("btnFindAllOpened")}},
        {1641, {QStringLiteral("btnFindAllCur")}},
        {1654, {QStringLiteral("lblFilters")}},
        {1655, {QStringLiteral("lblDirectory")}},
        {1656, {QStringLiteral("btnFindAllFif"), QStringLiteral("btnFindAllFip")}},
        {1658, {QStringLiteral("chkRecursive")}},
        {1659, {QStringLiteral("chkInHiddenDir")}},
        {1660, {QStringLiteral("btnReplaceInFiles")}},
        {1661, {QStringLiteral("chkFollowDoc")}},
        {1662, {QStringLiteral("chkProjectPanel1")}},
        {1663, {QStringLiteral("chkProjectPanel2")}},
        {1664, {QStringLiteral("chkProjectPanel3")}},
        {1665, {QStringLiteral("btnReplaceInProjects")}},
        {1686, {QStringLiteral("grpTransparency")}},
        {1687, {QStringLiteral("rbTransOnLostFocus")}},
        {1688, {QStringLiteral("rbTransAlways")}},
        {1703, {QStringLiteral("chkDotMatchNewline")}},
        {1722, {QStringLiteral("chkBackwardDir")}},
        {1725, {QStringLiteral("btnCopyMarked")}}
    };
    return ids.value(originalId);
}

static QStringList preferenceDialogObjectNames(int originalId)
{
    static const QMap<int, QStringList> ids = {
        {6001, {QStringLiteral("btnPrefsClose")}},
        {6851, {QStringLiteral("grpAutoInsert")}}
    };
    return ids.value(originalId);
}

static void collectPreferenceDialogItems(
    const TiXmlElement* parent, QMap<QString, QString>& widgets)
{
    if (!parent)
        return;
    for (const TiXmlElement* item = parent->FirstChildElement();
         item; item = item->NextSiblingElement()) {
        if (QString::fromWCharArray(item->Value()) == QLatin1String("Item")) {
            bool ok = false;
            const int id = txAttr(item, L"id").toInt(&ok);
            const QString name = txAttr(item, L"name");
            if (ok && !name.isEmpty()) {
                for (const QString& objectName : preferenceDialogObjectNames(id))
                    widgets[objectName] = name;
            }
        }
        collectPreferenceDialogItems(item, widgets);
    }
}

static const wchar_t* stableAttribute(const TiXmlElement* element)
{
    static const wchar_t* names[] = {
        L"id", L"menuId", L"subMenuId", L"CMID", L"idName", L"order"
    };
    for (const wchar_t* name : names) {
        if (element && element->Attribute(name))
            return name;
    }
    return nullptr;
}

static const TiXmlElement* matchingChild(const TiXmlElement* targetParent,
                                         const TiXmlElement* sourceChild,
                                         int sameTagIndex)
{
    if (!targetParent || !sourceChild)
        return nullptr;
    const wchar_t* tag = sourceChild->Value();
    if (const wchar_t* attribute = stableAttribute(sourceChild)) {
        const QString expected = txAttr(sourceChild, attribute);
        for (const TiXmlElement* candidate = targetParent->FirstChildElement(tag);
             candidate; candidate = candidate->NextSiblingElement(tag)) {
            if (txAttr(candidate, attribute) == expected)
                return candidate;
        }
        return nullptr;
    }
    const TiXmlElement* candidate = targetParent->FirstChildElement(tag);
    for (int index = 0; candidate && index < sameTagIndex; ++index)
        candidate = candidate->NextSiblingElement(tag);
    return candidate;
}

static void collectTranslationPairs(
    const TiXmlElement* source, const TiXmlElement* target,
    QMap<QString, QString>& exact, QMap<QString, QString>& normalized)
{
    if (!source || !target)
        return;
    static const wchar_t* textAttributes[] = {
        L"name", L"title", L"titleFind", L"titleReplace",
        L"titleFindInFiles", L"titleFindInProjects", L"titleMark",
        L"message", L"value", L"label"
    };
    for (const wchar_t* attribute : textAttributes) {
        const QString sourceText = txAttr(source, attribute);
        const QString targetText = txAttr(target, attribute);
        if (sourceText.isEmpty() || targetText.isEmpty())
            continue;
        if (!exact.contains(sourceText))
            exact.insert(sourceText, targetText);
        const QString key = normalizedNativeText(sourceText);
        if (!key.isEmpty() && !normalized.contains(key))
            normalized.insert(key, targetText);
    }

    QMap<QString, int> tagIndexes;
    for (const TiXmlElement* sourceChild = source->FirstChildElement();
         sourceChild; sourceChild = sourceChild->NextSiblingElement()) {
        const QString tag = QString::fromWCharArray(sourceChild->Value());
        const int index = tagIndexes.value(tag, 0);
        tagIndexes[tag] = index + 1;
        collectTranslationPairs(
            sourceChild, matchingChild(target, sourceChild, index),
            exact, normalized);
    }
}

// ── 从 Qt 资源或文件系统加载 TiXmlDocument ───────────────────────────────────

static bool loadXmlDoc(TiXmlDocument& doc, const QString& path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
        return false;
    QByteArray data = file.readAll();
    file.close();
    // Qt 正确处理 UTF-8 → wstring → TiXml 解析
    std::wstring wContent = QString::fromUtf8(data).toStdWString();
    doc.Parse(wContent.c_str());
    return !doc.Error();
}

// ── bool init() ─────────────────────────────────────────────────────────────

bool NativeLangSpeaker::init(const QString& xmlPath,
                             const QString& englishXmlPath)
{
    _loaded = false;
    _menuEntries.clear();
    _commands.clear();
    _strings.clear();
    _sourceTranslations.clear();
    _normalizedSourceTranslations.clear();
    _dialogs.clear();
    _dialogTitles.clear();

    if (xmlPath.isEmpty())
        return true;  // 空路径 = 英文模式，使用内建 tr() 文本

    TiXmlDocument targetDocument;
    TiXmlDocument englishDocument;
    if (englishXmlPath.isEmpty()
        || !loadXmlDoc(targetDocument, xmlPath)
        || !loadXmlDoc(englishDocument, englishXmlPath))
        return false;

    // 根节点 <NotepadPlus>
    const TiXmlElement* targetRoot = targetDocument.RootElement();
    const TiXmlElement* englishRoot = englishDocument.RootElement();
    if (!targetRoot || !englishRoot) return false;

    // <Native-Langue ...>
    const TiXmlElement* nativeLang =
        targetRoot->FirstChildElement(L"Native-Langue");
    const TiXmlElement* englishLang =
        englishRoot->FirstChildElement(L"Native-Langue");
    if (!nativeLang || !englishLang) return false;

    collectTranslationPairs(englishLang, nativeLang, _sourceTranslations,
                            _normalizedSourceTranslations);

    // ── <Menu> <Main> ────────────────────────────────────────────────────────
    const TiXmlElement* menuElem =
        nativeLang->FirstChildElement(L"Menu");
    if (menuElem) {
        const TiXmlElement* mainElem =
            menuElem->FirstChildElement(L"Main");
        if (mainElem) {
            // <Entries> — 顶级菜单名（对应原版 menuId）
            for (const TiXmlElement* entries =
                     mainElem->FirstChildElement(L"Entries");
                 entries; entries = entries->NextSiblingElement(L"Entries")) {
                for (const TiXmlElement* item =
                         entries->FirstChildElement(L"Item");
                     item; item = item->NextSiblingElement(L"Item"))
                {
                    const QString objectName =
                        menuObjectName(txAttr(item, L"menuId"));
                    const QString name = txAttr(item, L"name");
                    if (!objectName.isEmpty() && !name.isEmpty())
                        _menuEntries[objectName] = name;
                }
            }

            for (const TiXmlElement* entries =
                     mainElem->FirstChildElement(L"SubEntries");
                 entries; entries = entries->NextSiblingElement(L"SubEntries")) {
                for (const TiXmlElement* item =
                         entries->FirstChildElement(L"Item");
                     item; item = item->NextSiblingElement(L"Item")) {
                    const QString objectName =
                        menuObjectName(txAttr(item, L"subMenuId"));
                    const QString name = txAttr(item, L"name");
                    if (!objectName.isEmpty() && !name.isEmpty())
                        _menuEntries[objectName] = name;
                }
            }

            // <Commands> — 菜单项（对应原版整数 id，Qt 版用 objectName）
            for (const TiXmlElement* cmds =
                     mainElem->FirstChildElement(L"Commands");
                 cmds; cmds = cmds->NextSiblingElement(L"Commands")) {
                for (const TiXmlElement* item =
                         cmds->FirstChildElement(L"Item");
                     item; item = item->NextSiblingElement(L"Item"))
                {
                    bool ok = false;
                    const int commandId = txAttr(item, L"id").toInt(&ok);
                    const QString name = txAttr(item, L"name");
                    if (!ok || name.isEmpty())
                        continue;
                    for (const NppCommandMapping& mapping : nppCommandMappings()) {
                        if (mapping.commandId == commandId)
                            _commands[QString::fromLatin1(mapping.objectName)] = name;
                    }
                }
            }
        }
    }

    // ── <Strings> — 通用字符串 ───────────────────────────────────────────────
    const TiXmlElement* miscStrings =
        nativeLang->FirstChildElement(L"MiscStrings");
    if (miscStrings) {
        for (const TiXmlElement* item = miscStrings->FirstChildElement();
             item; item = item->NextSiblingElement()) {
            const QString id = QString::fromWCharArray(item->Value());
            const QString value = txAttr(item, L"value");
            if (!id.isEmpty() && !value.isEmpty())
                _strings[id] = value;
        }
    }

    // 原版 Find 对话框按数字控件 ID 本地化。Qt 层只在此处
    // 将这些 ID 连接到对应 objectName。
    const TiXmlElement* dialogRoot =
        nativeLang->FirstChildElement(L"Dialog");
    const TiXmlElement* findDialog = dialogRoot
        ? dialogRoot->FirstChildElement(L"Find") : nullptr;
    if (findDialog) {
        QMap<QString, QString>& widgets = _dialogs[QStringLiteral("FindReplace")];
        for (const TiXmlElement* item = findDialog->FirstChildElement(L"Item");
             item; item = item->NextSiblingElement(L"Item")) {
            bool ok = false;
            const int id = txAttr(item, L"id").toInt(&ok);
            const QString name = txAttr(item, L"name");
            if (!ok || name.isEmpty())
                continue;
            for (const QString& objectName : findDialogObjectNames(id))
                widgets[objectName] = name;
        }
    }
    const TiXmlElement* preferenceDialog = dialogRoot
        ? dialogRoot->FirstChildElement(L"Preference") : nullptr;
    if (preferenceDialog) {
        collectPreferenceDialogItems(
            preferenceDialog, _dialogs[QStringLiteral("Preferences")]);
    }

    // 原版关闭文档保存确认框：标题和正文来自 DoSaveOrNot，按钮
    // 继续沿用其中的 Win32 ID（IDYES/IDNO/IDCANCEL）。
    const TiXmlElement* doSaveDialog = dialogRoot
        ? dialogRoot->FirstChildElement(L"DoSaveOrNot") : nullptr;
    if (doSaveDialog) {
        const QString dialogName = QStringLiteral("DoSaveOrNot");
        _dialogTitles[dialogName] = txAttr(doSaveDialog, L"title");
        QMap<QString, QString>& items = _dialogs[dialogName];
        for (const TiXmlElement* item =
                 doSaveDialog->FirstChildElement(L"Item");
             item; item = item->NextSiblingElement(L"Item")) {
            bool ok = false;
            const int id = txAttr(item, L"id").toInt(&ok);
            const QString name = txAttr(item, L"name");
            if (ok && !name.isEmpty())
                items[QString::number(id)] = name;
        }
    }

    // ── <Dialog> 段 ──────────────────────────────────────────────────────────
    _loaded = true;
    return true;
}

QString NativeLangSpeaker::translateSource(const QString& source) const
{
    const auto exact = _sourceTranslations.constFind(source);
    if (exact != _sourceTranslations.cend())
        return exact.value();
    const auto normalized = _normalizedSourceTranslations.constFind(
        normalizedNativeText(source));
    return normalized == _normalizedSourceTranslations.cend()
        ? QString() : normalized.value();
}

// ── changeMenuLang() ─────────────────────────────────────────────────────────
// 对应原版 NativeLangSpeaker::changeMenuLang(HMENU menuHandle)
// 遍历主窗口的所有 QMenu / QAction，按 objectName 从 XML 中查找并替换文本

void NativeLangSpeaker::changeMenuLang(QMainWindow* win)
{
    if (!_loaded || !win) return;

    // 首次调用时保存原始（英文）文本，供切换回英文时用 restoreOriginalLang() 恢复
    if (!_hasOriginals) {
        for (QMenu* menu : win->menuBar()->findChildren<QMenu*>()) {
            const QString on = menu->objectName();
            if (!on.isEmpty())
                _origMenuTitles[on] = menu->title();
        }
        for (QAction* action : win->findChildren<QAction*>()) {
            const QString on = action->objectName();
            if (!on.isEmpty())
                _origActionTexts[on] = action->text();
        }
        for (QDockWidget* dock : win->findChildren<QDockWidget*>()) {
            const QString on = dock->objectName();
            if (!on.isEmpty())
                _origDockTitles[on] = dock->windowTitle();
        }
        for (QToolBar* toolBar : win->findChildren<QToolBar*>()) {
            const QString on = toolBar->objectName();
            if (!on.isEmpty())
                _origToolBarTitles[on] = toolBar->windowTitle();
        }
        _hasOriginals = true;
    }

    // 1. 顶级菜单（Entries）：objectName = menuId
    for (QMenu* menu : win->menuBar()->findChildren<QMenu*>()) {
        const QString on = menu->objectName();
        if (on.isEmpty()) continue;
        auto it = _menuEntries.find(on);
        if (it != _menuEntries.end())
            menu->setTitle(it.value());
        else if (const QString translated =
                     translateSource(_origMenuTitles.value(on));
                 !translated.isEmpty())
            menu->setTitle(translated);
    }

    // 2. 菜单动作（Commands）：objectName = objectName
    for (QAction* action : win->findChildren<QAction*>()) {
        const QString on = action->objectName();
        if (on.isEmpty()) continue;
        auto it = _commands.find(on);
        if (it != _commands.end())
            action->setText(it.value());
        else if (const QString translated =
                     translateSource(_origActionTexts.value(on));
                 !translated.isEmpty())
            action->setText(translated);
    }

    // 3. QDockWidget 标题（存储在 Commands 中，用 objectName+Title 约定）
    for (QDockWidget* dock : win->findChildren<QDockWidget*>()) {
        const QString on = dock->objectName();
        if (on.isEmpty()) continue;
        QString titleKey = on + "Title";
        auto it = _commands.find(titleKey);
        if (it == _commands.end())
            it = _commands.find(on);
        if (it != _commands.end())
            dock->setWindowTitle(it.value());
        else if (const QString translated =
                     translateSource(_origDockTitles.value(on));
                 !translated.isEmpty())
            dock->setWindowTitle(translated);
    }

    for (QToolBar* toolBar : win->findChildren<QToolBar*>()) {
        const QString on = toolBar->objectName();
        auto it = _commands.find(on);
        if (it != _commands.end())
            toolBar->setWindowTitle(it.value());
        else if (const QString translated =
                     translateSource(_origToolBarTitles.value(on));
                 !translated.isEmpty())
            toolBar->setWindowTitle(translated);
    }
}

// ── restoreOriginalLang() ────────────────────────────────────────────────────
// 切换回英文时调用：将菜单/动作恢复为首次 changeMenuLang() 前的原始文本

void NativeLangSpeaker::restoreOriginalLang(QMainWindow* win)
{
    if (!_hasOriginals || !win) return;

    for (QMenu* menu : win->menuBar()->findChildren<QMenu*>()) {
        const QString on = menu->objectName();
        auto it = _origMenuTitles.find(on);
        if (it != _origMenuTitles.end())
            menu->setTitle(it.value());
    }
    for (QAction* action : win->findChildren<QAction*>()) {
        const QString on = action->objectName();
        auto it = _origActionTexts.find(on);
        if (it != _origActionTexts.end())
            action->setText(it.value());
    }
    for (QDockWidget* dock : win->findChildren<QDockWidget*>()) {
        const QString on = dock->objectName();
        auto it = _origDockTitles.find(on);
        if (it != _origDockTitles.end())
            dock->setWindowTitle(it.value());
    }
    for (QToolBar* toolBar : win->findChildren<QToolBar*>()) {
        const QString on = toolBar->objectName();
        auto it = _origToolBarTitles.find(on);
        if (it != _origToolBarTitles.end())
            toolBar->setWindowTitle(it.value());
    }
}

// ── changeDlgLang() ──────────────────────────────────────────────────────────
// 对应原版 NativeLangSpeaker::changeDlgLang(HWND hDlg, const char* dlgTagName)

bool NativeLangSpeaker::changeDlgLang(QWidget* dlg, const QString& dlgTagName,
                                       QString* title)
{
    if (!_loaded || !dlg) return false;

    Q_UNUSED(dlgTagName)
    constexpr const char* originalTextProperty =
        "_nppNativeLangOriginalText";
    constexpr const char* originalItemsProperty =
        "_nppNativeLangOriginalItems";

    auto originalText = [](QObject* object, const char* property,
                           const QString& current) {
        const QVariant saved = object->property(property);
        if (saved.isValid())
            return saved.toString();
        object->setProperty(property, current);
        return current;
    };
    auto translated = [this](const QString& source) {
        const QString result = translateSource(source);
        return result.isEmpty() ? source : result;
    };
    auto originalItems = [](QObject* object, const char* property,
                            const QStringList& current) {
        const QVariant saved = object->property(property);
        if (saved.isValid())
            return saved.toStringList();
        object->setProperty(property, current);
        return current;
    };

    QString sourceTitle = title ? *title : dlg->windowTitle();
    sourceTitle = originalText(dlg, "_nppNativeLangOriginalTitle", sourceTitle);
    QString targetTitle = translateSource(sourceTitle);
    if (dlgTagName == QLatin1String("FindReplace")) {
        const QString find = translated(QStringLiteral("Find"));
        const QString replace = translated(QStringLiteral("Replace"));
        targetTitle = find + QStringLiteral(" / ") + replace;
    }
    if (!targetTitle.isEmpty()) {
        if (title)
            *title = targetTitle;
        else
            dlg->setWindowTitle(targetTitle);
    }

    for (QLabel* widget : dlg->findChildren<QLabel*>())
        widget->setText(translated(originalText(
            widget, originalTextProperty, widget->text())));
    for (QGroupBox* widget : dlg->findChildren<QGroupBox*>())
        widget->setTitle(translated(originalText(
            widget, originalTextProperty, widget->title())));
    for (QAbstractButton* widget : dlg->findChildren<QAbstractButton*>())
        widget->setText(translated(originalText(
            widget, originalTextProperty, widget->text())));
    for (QSpinBox* widget : dlg->findChildren<QSpinBox*>())
        widget->setSuffix(translated(originalText(
            widget, "_nppNativeLangOriginalSuffix", widget->suffix())));

    for (QListWidget* widget : dlg->findChildren<QListWidget*>()) {
        QStringList current;
        for (int i = 0; i < widget->count(); ++i)
            current.append(widget->item(i)->text());
        const QStringList source = originalItems(
            widget, originalItemsProperty, current);
        for (int i = 0; i < widget->count() && i < source.size(); ++i)
            widget->item(i)->setText(translated(source.at(i)));
    }
    for (QTabBar* widget : dlg->findChildren<QTabBar*>()) {
        QStringList current;
        for (int i = 0; i < widget->count(); ++i)
            current.append(widget->tabText(i));
        const QStringList source = originalItems(
            widget, originalItemsProperty, current);
        for (int i = 0; i < widget->count() && i < source.size(); ++i)
            widget->setTabText(i, translated(source.at(i)));
    }
    for (QComboBox* widget : dlg->findChildren<QComboBox*>()) {
        QStringList current;
        for (int i = 0; i < widget->count(); ++i)
            current.append(widget->itemText(i));
        const QStringList source = originalItems(
            widget, originalItemsProperty, current);
        for (int i = 0; i < widget->count() && i < source.size(); ++i)
            widget->setItemText(i, translated(source.at(i)));
    }
    for (QTableWidget* widget : dlg->findChildren<QTableWidget*>()) {
        QStringList current;
        for (int i = 0; i < widget->columnCount(); ++i) {
            const QTableWidgetItem* header = widget->horizontalHeaderItem(i);
            current.append(header ? header->text() : QString());
        }
        const QStringList source = originalItems(
            widget, originalItemsProperty, current);
        for (int i = 0; i < widget->columnCount() && i < source.size(); ++i) {
            if (QTableWidgetItem* header = widget->horizontalHeaderItem(i))
                header->setText(translated(source.at(i)));
        }
    }
    for (QTreeWidget* widget : dlg->findChildren<QTreeWidget*>()) {
        QStringList current;
        for (int i = 0; i < widget->topLevelItemCount(); ++i)
            current.append(widget->topLevelItem(i)->text(0));
        const QStringList source = originalItems(
            widget, originalItemsProperty, current);
        for (int i = 0; i < widget->topLevelItemCount() && i < source.size(); ++i)
            widget->topLevelItem(i)->setText(0, translated(source.at(i)));
    }

    const auto dialogTranslations = _dialogs.constFind(dlgTagName);
    if (dialogTranslations != _dialogs.cend()) {
        for (auto item = dialogTranslations->cbegin();
             item != dialogTranslations->cend(); ++item) {
            for (QLabel* widget : dlg->findChildren<QLabel*>(item.key()))
                widget->setText(item.value());
            for (QGroupBox* widget : dlg->findChildren<QGroupBox*>(item.key()))
                widget->setTitle(item.value());
            for (QAbstractButton* widget :
                 dlg->findChildren<QAbstractButton*>(item.key())) {
                widget->setText(item.value());
            }
        }
    }

    return true;
}

// ── getNativeLangMenuString() ────────────────────────────────────────────────

QString NativeLangSpeaker::getNativeLangMenuString(const QString& objectName) const
{
    auto it = _commands.find(objectName);
    if (it != _commands.end()) return it.value();
    return QString();
}

// ── getLocalizedStrFromID() ──────────────────────────────────────────────────

QString NativeLangSpeaker::getLocalizedStrFromID(const QString& id,
                                                  const QString& defaultStr) const
{
    auto it = _strings.find(id);
    if (it != _strings.end()) return it.value();
    return defaultStr;
}

bool NativeLangSpeaker::getDoSaveOrNotStrings(QString& title,
                                               QString& message) const
{
    if (!_loaded)
        return false;
    const QString dialogName = QStringLiteral("DoSaveOrNot");
    const QString localizedTitle = _dialogTitles.value(dialogName);
    const QString localizedMessage =
        _dialogs.value(dialogName).value(QStringLiteral("1761"));
    if (localizedTitle.isEmpty() || localizedMessage.isEmpty())
        return false;
    title = localizedTitle;
    message = localizedMessage;
    return true;
}

QString NativeLangSpeaker::getDialogItemText(
    const QString& dialogName, int id, const QString& defaultText) const
{
    return _dialogs.value(dialogName).value(QString::number(id), defaultText);
}
