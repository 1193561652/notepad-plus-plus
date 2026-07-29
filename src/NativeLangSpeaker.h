// NativeLangSpeaker.h - Qt 移植版本
// 对应原版 localization.h 的 NativeLangSpeaker 类
// 原版：Win32 HMENU/HWND + 整数命令 ID
// Qt版：objectName 字符串 + QAction/QMenu/QWidget

#ifndef NATIVELANGSPEAKER_H
#define NATIVELANGSPEAKER_H

#include <QString>
#include <QMap>

class QMainWindow;
class QWidget;

/**
 * NativeLangSpeaker - Qt 版本
 *
 * 从 XML 语言文件读取翻译，运行时直接应用到 UI 控件，无需重启。
 * 与原版相同：在 createMenus() 之后调用 changeMenuLang()；
 * 在对话框显示前调用 changeDlgLang()。
 *
 * XML 格式与原版一致，但用 objectName 代替 Win32 整数 ID：
 *   <Commands><Item objectName="newAction" name="新建(&amp;N)"/></Commands>
 *   <Entries> <Item menuId="fileMenu"      name="文件(&amp;F)"/></Entries>
 */
class NativeLangSpeaker
{
public:
    NativeLangSpeaker() = default;

    // 从文件路径（或 Qt 资源路径 ":/..."）加载语言文件
    // 加载英语时传空字符串，恢复内建 tr() 文本
    bool init(const QString& xmlPath);

    // 对应原版 changeMenuLang()：遍历主窗口所有 QMenu / QAction，按 objectName 替换文本
    // 首次调用时自动保存原始（英文）文本，供切换回英文时恢复
    void changeMenuLang(QMainWindow* win);

    // 切换回英文时调用：将菜单/动作文本恢复为首次调用 changeMenuLang() 前的原始值
    void restoreOriginalLang(QMainWindow* win);

    // 对应原版 changeDlgLang()：遍历对话框子控件，按 objectName 替换文本
    // dlgTagName 对应 XML 中 <Dialog name="..."> 的 name 属性
    // 如果 title 非空，写入对话框标题翻译
    bool changeDlgLang(QWidget* dlg, const QString& dlgTagName, QString* title = nullptr);

    // 对应原版 getNativeLangMenuString()：用 objectName 查找命令文本
    QString getNativeLangMenuString(const QString& objectName) const;

    // 对应原版 getLocalizedStrFromID()：用 id 查找通用字符串
    QString getLocalizedStrFromID(const QString& id, const QString& defaultStr) const;

    bool isLoaded() const { return _loaded; }

private:
    bool _loaded = false;

    // 菜单条目翻译（menuId → text），对应 <Entries> 段
    QMap<QString, QString> _menuEntries;

    // 命令/动作翻译（objectName → text），对应 <Commands> 段
    QMap<QString, QString> _commands;

    // 通用字符串（id → text），对应 <Strings> 段
    QMap<QString, QString> _strings;

    // 对话框子控件翻译（dlgName → （objectName → text）），对应 <Dialog> 段
    QMap<QString, QMap<QString, QString>> _dialogs;

    // 对话框标题翻译（dlgName → title）
    QMap<QString, QString> _dlgTitles;

    // 保存首次 changeMenuLang() 调用前的原始（英文）文本，用于切换回英文时恢复
    bool _hasOriginals = false;
    QMap<QString, QString> _origActionTexts;  // objectName → 原始动作文本
    QMap<QString, QString> _origMenuTitles;   // objectName → 原始菜单标题
    QMap<QString, QString> _origDockTitles;   // objectName → 原始停靠窗口标题
    QMap<QString, QString> _origToolBarTitles; // objectName → 原始工具栏标题
};

#endif // NATIVELANGSPEAKER_H
