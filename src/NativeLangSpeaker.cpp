// NativeLangSpeaker.cpp - Qt 移植版本
// 对应原版 localization.cpp

#include "NativeLangSpeaker.h"
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
#include <QFile>

// ── 内部辅助：宽字符属性读取 ─────────────────────────────────────────────────

static inline QString txAttr(const TiXmlElement* el, const wchar_t* name)
{
    const wchar_t* v = el ? el->Attribute(name) : nullptr;
    return v ? QString::fromWCharArray(v) : QString();
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

bool NativeLangSpeaker::init(const QString& xmlPath)
{
    _loaded = false;
    _menuEntries.clear();
    _commands.clear();
    _strings.clear();
    _dialogs.clear();
    _dlgTitles.clear();

    if (xmlPath.isEmpty())
        return true;  // 空路径 = 英文模式，使用内建 tr() 文本

    TiXmlDocument doc;
    if (!loadXmlDoc(doc, xmlPath))
        return false;

    // 根节点 <NotepadPlus>
    const TiXmlElement* root = doc.RootElement();
    if (!root) return false;

    // <Native-Langue ...>
    const TiXmlElement* nativeLang =
        root->FirstChildElement(L"Native-Langue");
    if (!nativeLang) return false;

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
                    QString id   = txAttr(item, L"menuId");
                    QString name = txAttr(item, L"name");
                    if (!id.isEmpty() && !name.isEmpty())
                        _menuEntries[id] = name;
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
                    QString oname = txAttr(item, L"objectName");
                    QString name  = txAttr(item, L"name");
                    if (!oname.isEmpty() && !name.isEmpty())
                        _commands[oname] = name;
                }
            }
        }
    }

    // ── <Strings> — 通用字符串 ───────────────────────────────────────────────
    const TiXmlElement* stringsElem =
        nativeLang->FirstChildElement(L"Strings");
    if (stringsElem) {
        for (const TiXmlElement* item =
                 stringsElem->FirstChildElement(L"Item");
             item; item = item->NextSiblingElement(L"Item"))
        {
            QString id   = txAttr(item, L"id");
            QString name = txAttr(item, L"name");
            if (!id.isEmpty() && !name.isEmpty())
                _strings[id] = name;
        }
    }

    // ── <Dialog> 段 ──────────────────────────────────────────────────────────
    for (const TiXmlElement* dlg =
             nativeLang->FirstChildElement(L"Dialog");
         dlg; dlg = dlg->NextSiblingElement(L"Dialog"))
    {
        QString dlgName  = txAttr(dlg, L"name");
        QString dlgTitle = txAttr(dlg, L"title");
        if (dlgName.isEmpty()) continue;

        if (!dlgTitle.isEmpty())
            _dlgTitles[dlgName] = dlgTitle;

        QMap<QString, QString> widgetMap;
        for (const TiXmlElement* item =
                 dlg->FirstChildElement(L"Item");
             item; item = item->NextSiblingElement(L"Item"))
        {
            QString oname = txAttr(item, L"objectName");
            QString name  = txAttr(item, L"name");
            if (!oname.isEmpty() && !name.isEmpty())
                widgetMap[oname] = name;
        }
        _dialogs[dlgName] = widgetMap;
    }

    _loaded = true;
    return true;
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
    }

    // 2. 菜单动作（Commands）：objectName = objectName
    for (QAction* action : win->findChildren<QAction*>()) {
        const QString on = action->objectName();
        if (on.isEmpty()) continue;
        auto it = _commands.find(on);
        if (it != _commands.end())
            action->setText(it.value());
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
    }

    for (QToolBar* toolBar : win->findChildren<QToolBar*>()) {
        const QString on = toolBar->objectName();
        auto it = _commands.find(on);
        if (it != _commands.end())
            toolBar->setWindowTitle(it.value());
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

    // 对话框标题
    if (title) {
        auto it = _dlgTitles.find(dlgTagName);
        if (it != _dlgTitles.end())
            *title = it.value();
    } else {
        auto it = _dlgTitles.find(dlgTagName);
        if (it != _dlgTitles.end())
            dlg->setWindowTitle(it.value());
    }

    auto it = _dialogs.find(dlgTagName);
    if (it == _dialogs.end()) return false;

    const QMap<QString, QString>& widgetMap = it.value();

    // 遍历所有带 objectName 的子控件，按类型调用对应的 setText
    auto apply = [&](const QString& on, const QString& text) {
        // QLabel
        for (QLabel* w : dlg->findChildren<QLabel*>(on))
            w->setText(text);
        // QGroupBox
        for (QGroupBox* w : dlg->findChildren<QGroupBox*>(on))
            w->setTitle(text);
        // QCheckBox / QRadioButton / QPushButton (QAbstractButton)
        for (QAbstractButton* w : dlg->findChildren<QAbstractButton*>(on))
            w->setText(text);
    };

    for (auto jt = widgetMap.begin(); jt != widgetMap.end(); ++jt) {
        const QString& on   = jt.key();
        const QString& text = jt.value();

        if (on.endsWith(QLatin1String("_suffix"))) {
            const QString widgetName = on.left(on.size() - 7);
            for (QSpinBox* spinBox : dlg->findChildren<QSpinBox*>(widgetName))
                spinBox->setSuffix(text);
            continue;
        }

        // 列表、标签页和下拉框条目：约定 "<widgetName>_<index>" 格式
        int underscorePos = on.lastIndexOf(QLatin1Char('_'));
        if (underscorePos > 0) {
            bool ok = false;
            int itemIdx = on.mid(underscorePos + 1).toInt(&ok);
            if (ok) {
                QString widgetName = on.left(underscorePos);
                // QListWidget 条目
                for (QListWidget* lw : dlg->findChildren<QListWidget*>(widgetName)) {
                    if (itemIdx >= 0 && itemIdx < lw->count())
                        lw->item(itemIdx)->setText(text);
                }
                // QTabBar 标签
                for (QTabBar* tb : dlg->findChildren<QTabBar*>(widgetName)) {
                    if (itemIdx >= 0 && itemIdx < tb->count())
                        tb->setTabText(itemIdx, text);
                }
                for (QComboBox* combo : dlg->findChildren<QComboBox*>(widgetName)) {
                    if (itemIdx >= 0 && itemIdx < combo->count())
                        combo->setItemText(itemIdx, text);
                }
            }
        }
        apply(on, text);
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
