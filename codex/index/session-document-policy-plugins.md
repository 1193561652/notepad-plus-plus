# 代码索引：Session 与文档策略插件

## 入口

- 包与 CTest：`CMakeLists.txt`、`tests/Win32PluginCorpusInstallPlan.json.in`
- 功能验证：`tests/UiParityCapture.cpp`
- NPP ABI 常量：`src/Win32PluginSystem/Win32PluginInterface.h`
- 主窗口消息：`src/Win32PluginSystem/Win32MainWindowAdapter.cpp`
- SCI ABI：`src/Win32PluginSystem/Win32EditorMessageAdapter.*`
- 插件回调 ABI 选择：`src/Win32PluginSystem/Win32PluginManager.*`
- session 宿主服务：`src/MainWindow.*`、`src/NppIO.cpp`
- 编码命令 ID：`src/NppCommands.cpp`
- 文件通知顺序：`src/NppIO.cpp`、`src/NppNotification.cpp`

## 固定语料

| 插件 | 版本 | SHA-256 |
|---|---:|---|
| SessionMgr | 1.4.4 | `478f7057863dd998b26c37083d785106a65853063d820b365ec18a02e755da9d` |
| AutoCodepage | 1.2.4 | `88744cda8c4a6be373af2fa23c86156adaf3dc000fde8834957f9352a6f34fec` |
| AutoEolFormat | 1.0.2 | `9488d63c983050d65af7911d3a3f19dbba6d3fc5a14ad1660ecfbadf31343a85` |
| nppAutoDetectIndent | 2.3 | `5d1982a3fb4609d74dde0deaf631526646d393fc26e6d7a11c72f9d0456d69f1` |

## 关键约束

- 不修改官方 DLL。
- 插件专属 Win32 ABI 留在 `Win32PluginSystem`。
- 文件打开通知顺序属于宿主公共契约，不得因 Qt 信号时序改变。
- `SCI_GETTEXTRANGE` 的结构布局取决于插件使用的 Scintilla ABI，不能全局固定翻译。

