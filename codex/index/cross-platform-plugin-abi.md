# 跨平台插件 ABI 代码索引

## 公共接口

- `src/CrossPlatformPluginSystem/PluginInterface.h`：ABI v1 唯一公共头文件。
- `src/CrossPlatformPluginSystem/README.md`：插件作者的导出、所有权、线程和包布局说明。

## 宿主实现

- `src/MISC/PluginsManager/PluginManager.*`：发现 ABI v1 动态库、解析六导出、建立命令表、
  发送生命周期通知并逆序卸载。
- `src/MISC/PluginsManager/PluginHostServices.*`：平台中性宿主服务门面；实现只委托
  `MainWindow` 和 `NppParameters` 的既有行为。
- `src/MISC/PluginsManager/PluginArtifactResolver.*`：先返回标准插件二进制路径，缺失时
  回退 ABI v1 早期的 `cross-platform/<platform>` 兼容路径。
- `src/Win32PluginSystem/Win32MainWindowAdapter.*`：NPPM/Win32 类型到同一宿主服务的
  薄适配，不拥有业务逻辑。
- `src/MainWindow.cpp::setupPluginSystem()`：读取统一启用配置，先加载 ABI v1，再把未由
  ABI v1 占用的目录交给 Windows 原版 DLL 管理器；同一 DLL 同时导出新旧 ABI 时新
  ABI 优先，只含旧导出时静默回退原逻辑。

## ABI v1 追加服务

- `get_current_document` / `replace_current_document`：显式长度的原始文档字节读写，支持
  嵌入 NUL；宿主写入使用 `SCI_ADDTEXT` / `SCI_REPLACETARGET`，不使用 C 字符串语义。
- `get_current_selection` / `replace_current_selection` / `set_current_selection`：选择区字节
  与位置操作。
- `get_current_buffer_id` / `get_current_view`：当前文档与主副视图标识。
- `create_document`、剪贴板和状态栏回调：复用宿主既有文档/UI 行为，不暴露 Qt 类型。
- `get_view_document` / `show_buffer_in_view`：按主副视图读取原始文档，并让已有 Buffer
  进入指定永久编辑器；插件不持有 `ScintillaEditView` 或 Qt 对象。
- `clear_compare_marks` / `add_compare_mark`：提供 Added、Removed、Changed、Moved 四类
  行背景标记；宿主使用保留的 Scintilla marker 10 至 13。
- `get_first_visible_line` / `set_first_visible_line` / `goto_line`：支持比较插件的滚动同步
  和差异导航，调用保持同步且只允许视图索引 0/1。
- 新字段只追加到 `NppPluginHostInfo` 尾部；旧 ABI v1 插件继续按自己的 `struct_size`
  使用原字段，新插件必须先检查宿主结构大小。

## 测试

- `tests/plugins/CrossPlatformAbiPlugin.cpp`：纯 C ABI 形态的最小真实共享库插件。
- `tests/CrossPlatformPluginTests.cpp`：环境、版本、二进制文档/选择区/剪贴板、双视图、
  标记、滚动和跳转回调，以及双 ABI 导出优先级、命令和生命周期验证。
- `tests/PluginAdminTests.cpp`：ABI v1 平台目录发现和已安装页识别。

修改公共头文件后必须至少运行 `cross-platform-plugin-tests`、`plugin-admin-tests`、
Win32 插件共存测试和完整 CTest。`NppPluginHostInfo` 新字段只能追加并通过
`struct_size` 判断；连续的 `NppPluginFuncItem` 数组改变布局时必须升级 ABI 版本。
