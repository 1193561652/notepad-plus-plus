# 插件源码仓库索引

## 工作区布局

以下插件源码是 Qt 主线仓库的兄弟目录，各自是独立 Git 仓库，不属于
`notepad-plus-plus` 的 CMake 源码树：

| 插件 | 相对 Qt 主线的本地路径 | Fork (`origin`) | 官方仓库 (`upstream`) | 官方默认分支 | 移植分支 |
| --- | --- | --- | --- | --- | --- |
| ComparePlus | `../comparePlus/` | `https://github.com/1193561652/comparePlus.git` | `https://github.com/pnedev/comparePlus.git` | `main` | `qt-port` |
| HEX-Editor | `../NPP_HexEditor/` | `https://github.com/1193561652/NPP_HexEditor.git` | `https://github.com/chcg/NPP_HexEditor.git` | `master` | `qt-port` |
| DSpellCheck | `../DSpellCheck/` | `https://github.com/1193561652/DSpellCheck.git` | `https://github.com/Predelnik/DSpellCheck.git` | upstream default | `qt-port` |
| MarkdownViewerPlusPlus | `../MarkdownViewerPlusPlus/` | `https://github.com/1193561652/MarkdownViewerPlusPlus.git` | `https://github.com/nea/MarkdownViewerPlusPlus.git` | upstream default | `qt-port` |
| Explorer | `../npp-explorer-plugin/` | `https://github.com/1193561652/npp-explorer-plugin.git` | `https://github.com/oviradoi/npp-explorer-plugin.git` | upstream default | `qt-port` |
| NppExec | `../nppexec/` | `https://github.com/1193561652/nppexec.git` | `https://github.com/d0vgan/nppexec.git` | upstream default | `qt-port` |
| NppFTP | `../NppFTP/` | `https://github.com/1193561652/NppFTP.git` | `https://github.com/ashkulz/NppFTP.git` | upstream default | `qt-port` |
| NppMarkdownPanel | `../NppMarkdownPanel/` | `https://github.com/1193561652/NppMarkdownPanel.git` | `https://github.com/mohzy83/NppMarkdownPanel.git` | upstream default | `qt-port` |
| NPPTextFX2 | `../NPPTextFX2/` | `https://github.com/1193561652/NPPTextFX2.git` | `https://github.com/rainman74/NPPTextFX2.git` | upstream default | `qt-port` |
| PythonScript | `../PythonScript/` | `https://github.com/1193561652/PythonScript.git` | `https://github.com/bruderstein/PythonScript.git` | upstream default | `qt-port` |

在当前工作区中的绝对路径分别是：

- `F:/project2/Notepad++/comparePlus/`
- `F:/project2/Notepad++/NPP_HexEditor/`
- `F:/project2/Notepad++/DSpellCheck/`
- `F:/project2/Notepad++/MarkdownViewerPlusPlus/`
- `F:/project2/Notepad++/npp-explorer-plugin/`
- `F:/project2/Notepad++/nppexec/`
- `F:/project2/Notepad++/NppFTP/`
- `F:/project2/Notepad++/NppMarkdownPanel/`
- `F:/project2/Notepad++/NPPTextFX2/`
- `F:/project2/Notepad++/PythonScript/`

绝对路径只用于定位当前工作区；代码、文档和脚本应优先使用上表中的相对路径。

## v8.4.6 对照基线

- ComparePlus：Notepad++ v8.4.6 插件清单版本为 `1.0.0`，源码基线 tag 为
  `cp_1.0.0`；`qt-port` 起点为提交 `cd943fa62a2cf58f550c948189cd79f1e40e47c8`。
- HEX-Editor：Notepad++ v8.4.6 插件清单版本为 `0.9.12.0`，源码基线 tag 为
  `0.9.12`；`qt-port` 起点为提交 `67350aaa8f6374408bb2bf942f17a67c0a5e64d7`。
- 分析或移植时先检出对应基线 tag，再与插件当前分支比较；不要直接以最新版本行为
  代替 v8.4.6 基线行为。

两个本地仓库当前均检出 `qt-port`，并跟踪各自的 `origin/qt-port`。

## 高优先级插件基线

| 插件 | 基线 tag | 起点提交 |
| --- | --- | --- |
| DSpellCheck | `v1.4.24` | `59ddfbb` |
| MarkdownViewerPlusPlus | `0.8.2` | `1a3dddc` |
| Explorer | `v1.9.5` | `1c4fa22` |
| NppExec | `v082` | `818ca67` |
| NppFTP | `v0.29.10` | `bf73f9f` |
| NppMarkdownPanel | `0.6.2` | `86cafce` |
| NPPTextFX2 | `1.2.1` | `4dd8d86` |
| PythonScript | `v2.0.0` | `accd4b14` |

以上仓库已从 `upstream` 获取官方 tag 并同步到各自 `origin`，当前均检出从基线
tag 创建的 `qt-port`。详细实现和差异见 `codex/index/high-priority-plugin-ports.md`。

## Qt 移植实现

- ComparePlus：`../comparePlus/qt/`，构建目标 `ComparePlusQt`，产物
  `ComparePlus-qt.dll`（其他平台使用对应动态库后缀）。
- HEX-Editor：`../NPP_HexEditor/qt/`，构建目标 `HexEditorQt`，产物
  `HexEditor-qt.dll`（其他平台使用对应动态库后缀）。
- 两个仓库均以 `build-qt/` 作为本地构建目录，并通过各自 CTest 的核心测试和 ABI
  动态加载烟雾测试。

## Tag 同步状态

2026-08-13 已从官方 `upstream` 获取并推送到用户 fork：

- ComparePlus：12 个 tag，范围包含 `cp_1.0.0` 至 `cp_3.0.0`，以及旧
  Compare-plugin 的 `v2.0.0` 至 `v2.0.2` 系列。
- HEX-Editor：12 个 tag，范围包含 `0.9.5.10` 至 `0.9.14`。

后续同步命令需在各插件仓库中执行：

```powershell
git fetch upstream --tags --prune
git push origin --tags
```

## 使用边界

- 两个目录用于阅读原版插件源码、建立接口/行为索引和开展独立插件移植。
- 未经明确的集成决策，不把插件仓库作为主程序源码目录或 Git 子模块。
- 修改插件前确认当前分支和 tag；基线分析尽量引用 tag 与 commit，避免引用浮动分支。
- 插件接口、宿主消息和适配结论同步更新 `codex/features/plugin-system.md` 及相关分析。
