# 高优先级 Qt 插件索引

## 接入规则

- 插件源码保持为 Qt 主仓库的同级独立 Git 仓库，原版源码目录不做结构性重写。
- Qt 实现集中在各仓库的 `qt/`，通过主程序跨平台插件 ABI v1 接入。
- 新 ABI 名称统一追加 `-qt`；Windows 构建同时导出原 Notepad++ ABI，但旧
  `getFuncsArray()` 固定返回空表，使原版应用加载时不注册命令。
- 主程序发现新旧 ABI 同时存在时优先调用新 ABI；仅存在旧 ABI 时才进入 Win32
  兼容路径。

## 构建与功能状态

| 插件 | Qt 目标/产物名 | 已覆盖的主要流程 | CTest |
| --- | --- | --- | --- |
| DSpellCheck | `DSpellCheck-qt` | Hunspell 字典、检查、前后跳转、建议替换、忽略和用户词典 | 2/2 |
| MarkdownViewerPlusPlus | `MarkdownViewerPlusPlus-qt` | Markdown dock 预览、扩展名过滤、CSS、HTML/PDF 导出 | 2/2 |
| Explorer | `Explorer-qt` | 文件树 dock、过滤、导航、打开文件、收藏夹、隐藏文件 | 1/1 |
| NppExec | `NppExec-qt` | 脚本解析、变量、命名脚本仓库、常用内部命令、外部进程、控制台 dock、SCI 指针消息 | 2/2 |
| NppFTP | `NppFTP-qt` | XML profile、远程列表、下载打开、上传当前文档、FTP/SFTP curl 后端 | 2/2 |
| NppMarkdownPanel | `NppMarkdownPanel-qt` | 预览 dock、CSS、缩放、工具栏、自动 HTML | 1/1 |
| NPPTextFX2 | `NPPTextFX2-qt` | 31 个常用文本转换、排序、编码、统计命令 | 2/2 |
| PythonScript | `PythonScript-qt` | 脚本菜单、控制台、外部 Python 运行和基础 editor/notepad bridge | 1/1 |

## 尚存的深层差异

- DSpellCheck：缺少实时 Scintilla 波浪线、右键建议、多语言/Aspell 和字典下载界面。
- 两个 Markdown 插件：当前解析器未覆盖原版全部 CommonMark/Markdig 扩展，编辑器
  与预览滚动同步仍需宿主事件接口。
- Explorer：收藏配置尚未复刻 `Favorites.dat`，缺少 Shell 上下文菜单和部分组操作。
- NppExec：已补齐原版 `npes_saved.txt` 命名脚本仓库、`NPP_EXEC` 和 SCI 指针参数；任意 NPP Win32 窗口消息不进入跨平台 ABI。
- NppFTP：缺少保存触发上传、密钥认证、完整传输队列/缓存和主密码加密。
- NPPTextFX2：覆盖常用命令，但不是原版约百项命令及自动编辑钩子的完整集合。
- PythonScript：目前是外部解释器桥，不是原版嵌入式 Python 与完整 Scintilla/Notepad API。

这些差异应优先通过追加平台无关宿主能力解决；不要把 Qt 对象或平台窗口句柄加入公共 ABI。

## 深层行为批次

2026-08-13 后续批次已完成以下基础设施和行为：

- ABI v1 尾部追加平台无关 Scintilla 消息、Buffer 路径、当前文件保存和菜单命令服务；
  通知增加文件生命周期、Buffer/语言变化、文本修改和 UI 更新及其负载。
- DSpellCheck 使用 Scintilla indicator 实时标记，编辑后防抖重检，并持久加载用户词典。
- 两个 Markdown 插件响应编辑/切换事件、同步滚动，并支持表格、任务列表、Setext 标题、
  围栏语言、缩进代码和自动链接。
- Explorer 读写原版 UTF-16LE `Favorites.dat` 链接记录，保留 INI 回退，并提供明确的
  打开/删除上下文菜单。
- NppExec 使用串行异步动作队列，支持非阻塞 SLEEP、错误位置跳转、保存全部、
  SEL/CON 文件读写、菜单命令和 Scintilla 消息。
- NppFTP 使用 FIFO 传输队列、哈希隔离缓存、保存后自动上传，以及 SFTP 私钥/口令。
- NPPTextFX2 增加 Base64、URI、数制、Hex/Text、ASCII 表、日期时间和清空撤销等命令。
- PythonScript 外部运行时桥增加选区、位置、行、查找、插入/删除及 open/new/save 请求。

仍不能宣称逐项完全等价：原版 PythonScript 的嵌入式 Python 2 运行时、Markdown 插件的
Markdig 二进制、NppFTP 主密码加密实现和 TextFX 的 x86-only Viz/Tidy 功能未包含在基线
源码依赖中。它们应作为独立第三方依赖/安全设计批次处理，而不是在兼容层内模拟。
