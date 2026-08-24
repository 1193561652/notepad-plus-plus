// localization.h - NativeLangSpeaker 的 Qt 平台接口
// 对应原版 localization.h 的 NativeLangSpeaker 类
// 原版：Win32 HMENU/HWND + 整数命令 ID
// Qt 版保留原版 XML 和 ID 逻辑，仅在控件应用层替换平台 API。

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
 * XML 文件与 Notepad++ v8.4.6 原版保持一致：菜单命令仍使用
 * 数字 ID，对话框仍使用原版节点。Qt objectName 不写入语言文件。
 */
class NativeLangSpeaker
{
public:
    NativeLangSpeaker() = default;

    // 从安装目录中的 XML 文件加载语言；英语 XML 用于原版节点配对。
    // 加载英语时传空字符串，恢复内建 tr() 文本
    bool init(const QString& xmlPath, const QString& englishXmlPath = QString());

    // 对应原版 changeMenuLang()：按原版菜单 ID 替换文本
    // 首次调用时自动保存原始（英文）文本，供切换回英文时恢复
    void changeMenuLang(QMainWindow* win);

    // 切换回英文时调用：将菜单/动作文本恢复为首次调用 changeMenuLang() 前的原始值
    void restoreOriginalLang(QMainWindow* win);

    // 对应原版 changeDlgLang()：从原版英文/目标语言节点配对文本，
    // 再在 Qt 控件层应用，不扩展 XML 格式。
    // dlgTagName 对应 XML 中 <Dialog name="..."> 的 name 属性
    // 如果 title 非空，写入对话框标题翻译
    bool changeDlgLang(QWidget* dlg, const QString& dlgTagName, QString* title = nullptr);

    // 对应原版 getNativeLangMenuString()：用 objectName 查找命令文本
    QString getNativeLangMenuString(const QString& objectName) const;

    // 对应原版 getLocalizedStrFromID()：用 id 查找通用字符串
    QString getLocalizedStrFromID(const QString& id, const QString& defaultStr) const;

    // 对应原版 getDoSaveOrNotStrings()，从
    // <Dialog><DoSaveOrNot> 读取关闭文档时的保存提示。
    bool getDoSaveOrNotStrings(QString& title, QString& message) const;
    QString getDialogItemText(const QString& dialogName, int id,
                              const QString& defaultText) const;

    bool isLoaded() const { return _loaded; }

private:
    bool _loaded = false;

    // Qt 菜单 objectName → 原版 XML 文本。键的映射仅属于平台层。
    QMap<QString, QString> _menuEntries;

    // 命令 ID 通过 NppCommandRegistry 映射到 QAction objectName。
    QMap<QString, QString> _commands;

    // 通用字符串（id → text），对应 <Strings> 段
    QMap<QString, QString> _strings;

    // 原版 english.xml 文本 → 当前 nativeLang.xml 文本。
    QMap<QString, QString> _sourceTranslations;
    QMap<QString, QString> _normalizedSourceTranslations;

    // 对话框数字 ID → Qt objectName 的平台适配结果。语言文件
    // 本身仍保留原版 ID。
    QMap<QString, QMap<QString, QString>> _dialogs;
    QMap<QString, QString> _dialogTitles;

    QString translateSource(const QString& source) const;

    // 保存首次 changeMenuLang() 调用前的原始（英文）文本，用于切换回英文时恢复
    bool _hasOriginals = false;
    QMap<QString, QString> _origActionTexts;  // objectName → 原始动作文本
    QMap<QString, QString> _origMenuTitles;   // objectName → 原始菜单标题
    QMap<QString, QString> _origDockTitles;   // objectName → 原始停靠窗口标题
    QMap<QString, QString> _origToolBarTitles; // objectName → 原始工具栏标题
};

#endif // NATIVELANGSPEAKER_H
