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
| DSpellCheck | `DSpellCheck-qt` | 多 Hunspell 语言、本地词典管理、完整右键建议、忽略和用户词典 | 2/2 |
| MarkdownViewerPlusPlus | `MarkdownViewerPlusPlus-qt` | CommonMark/GFM 与 Markdig AdvancedExtensions、CSS、HTML/PDF 导出 | 2/2 |
| Explorer | `Explorer-qt` | 文件树 dock、过滤、导航、打开文件、收藏夹、隐藏文件 | 1/1 |
| NppExec | `NppExec-qt` | 脚本解析、变量、命名脚本仓库、常用内部命令、外部进程、控制台 dock、SCI 指针消息 | 2/2 |
| NppFTP | `NppFTP-qt` | 主密码认证加密、可管理传输队列、持久缓存、FTP/SFTP curl 后端 | 2/2 |
| NppMarkdownPanel | `NppMarkdownPanel-qt` | CommonMark/GFM 与 Markdig AdvancedExtensions、CSS、缩放、自动 HTML | 1/1 |
| NPPTextFX2 | `NPPTextFX2-qt` | 84 个可移植文本转换、编辑、排序、编码、统计和插入命令 | 2/2 |
| PythonScript | `PythonScript-qt` | 嵌入式 Python 3、脚本菜单/控制台、完整 editor 消息 API、可移植 notepad 文件/Buffer/会话/通知 API | 2/2 |

## 尚存的深层差异

- DSpellCheck：Hunspell 基础流程已收口；Aspell 和在线词典下载不进入无网络依赖的
  Qt 版范围，本地 `.dic`/`.aff` 导入、删除和多语言选择由词典管理器负责。
- 两个 Markdown 插件：基础语法和原版 `UseAdvancedExtensions()` 已覆盖；图表围栏与
  数学公式严格生成原版静态 HTML 标记。原版没有内置或联网加载对应 JavaScript
  绘制引擎，Qt 版也不额外引入。
- Explorer：收藏配置尚未复刻 `Favorites.dat`，缺少 Shell 上下文菜单和部分组操作。
- NppExec：已补齐原版 `npes_saved.txt` 命名脚本仓库、`NPP_EXEC` 和 SCI 指针参数；任意 NPP Win32 窗口消息不进入跨平台 ABI。
- NppFTP：主密码、队列和缓存基础流程已收口；系统钥匙串和跨设备凭据同步不属于
  原版主密码模型。
- NPPTextFX2：原版可移植命令已补齐；旧 32 位 Viz、Win32 subclass/键盘钩子、外部
  Tidy DLL，以及依赖原始单字节文档缓冲区的 EBCDIC/KOI8 转码不进入 UTF-8 ABI。
- PythonScript：743 个 Scintilla 方法、2,306 个原始值及 775 个 SCI/SCN 标识已接入；
  `formatRange` 因包含原生绘图句柄明确不提供。可移植的文件、Buffer、会话、语言、编码、
  状态栏和宿主通知已接入；HWND、原始 Win32 消息、原生菜单句柄及隐藏 Scintilla 窗口
  等 Windows 专用接口不进入跨平台范围。

这些差异应优先通过追加平台无关宿主能力解决；不要把 Qt 对象或平台窗口句柄加入公共 ABI。

## 深层行为批次

2026-08-13 后续批次已完成以下基础设施和行为：

- ABI v1 尾部追加平台无关 Scintilla 消息、Buffer 路径、当前文件保存和菜单命令服务；
  通知增加文件生命周期、Buffer/语言变化、文本修改和 UI 更新及其负载。
- DSpellCheck 使用 Scintilla indicator 实时标记，编辑后防抖重检，合并多个 Hunspell
  语言的建议，并管理本地词典对和持久用户词典。
- 两个 Markdown 插件响应编辑/切换事件、同步滚动，并支持表格、任务列表、Setext 标题、
  围栏语言、缩进代码和自动链接。
- Explorer 读写原版 UTF-16LE `Favorites.dat` 链接记录，保留 INI 回退，并提供明确的
  打开/删除上下文菜单。
- NppExec 使用串行异步动作队列，支持非阻塞 SLEEP、错误位置跳转、保存全部、
  SEL/CON 文件读写、菜单命令和 Scintilla 消息。
- NppFTP 使用 FIFO 传输队列、哈希隔离缓存、保存后自动上传，以及 SFTP 私钥/口令。
- NPPTextFX2 增加 Base64、URI、数制、Hex/Text、UUdecode、引号反转义、HTML 表格、
  剪贴板分隔/对齐/重排、缩进、括号导航、复制和插入等命令。
- PythonScript 改为进程内嵌入 Python 3，`editor/editor1/editor2` 直接发送 Scintilla
  消息；方法和常量由原版 `Scintilla.iface` 生成，并为字符串、颜色、cells、文本范围、
  styled text、搜索和 string-result 参数提供平台无关适配。

仍不能宣称逐项完全等价：各插件明确排除的原生窗口句柄、Win32 钩子、32 位 Viz/Tidy
等能力不会在跨平台兼容层内模拟。PythonScript 本轮详情见
`codex/changes/2026-08-27-pythonscript-embedded-python-editor-api.md`。
本轮 DSpellCheck 与 TextFX 详情见
`codex/changes/2026-08-28-dspellcheck-textfx-completion.md`。
