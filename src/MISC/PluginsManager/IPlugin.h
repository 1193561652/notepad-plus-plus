// IPlugin.h - 插件接口定义
// 移植自: v8.4.6:PowerEditor/src/PluginsManager/PluginInterface.h

#ifndef IPLUGIN_H
#define IPLUGIN_H

#include <QString>
#include <QList>

class QAction;
class QMainWindow;
class ScintillaEditView;

// ─── 宿主接口（插件通过此接口访问编辑器功能） ────────────────────────────────

class IPluginHost
{
public:
    virtual ~IPluginHost() = default;

    // 获取当前活动编辑视图
    virtual ScintillaEditView* currentView() = 0;

    // 打开文件
    virtual void openFile(const QString& filePath) = 0;

    // 获取当前文件路径
    virtual QString currentFilePath() const = 0;

    // 获取主窗口（用于插件添加子窗口等）
    virtual QMainWindow* mainWindow() = 0;
};

// ─── 插件接口 ─────────────────────────────────────────────────────────────────

class IPlugin
{
public:
    virtual ~IPlugin() = default;

    virtual QString getName() const    = 0;
    virtual QString getVersion() const = 0;

    // 插件加载时调用
    virtual void init(IPluginHost* host) = 0;

    // 插件卸载时调用
    virtual void cleanup() = 0;

    // 返回该插件贡献的菜单动作列表（显示在 Plugins 菜单下）
    virtual QList<QAction*> getMenuActions() = 0;
};

// ─── 插件 DLL 导出函数类型 ────────────────────────────────────────────────────
// 插件 DLL 必须导出名为 createPlugin / destroyPlugin 的 C 函数

extern "C" {
    typedef IPlugin* (*CreatePluginFn)();
    typedef void     (*DestroyPluginFn)(IPlugin*);
}

#endif // IPLUGIN_H
