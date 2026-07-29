# 命令行参数兼容

更新时间：2026-07-28

## 状态

- Notepad++ v8.4.6 命令行参数全集已接入 Qt 主线。
- 参数解析、启动前配置覆盖、启动窗口行为、文件级行为和单实例转发使用同一数据模型。
- 插件参数保留并可通过 IPC 转发；插件系统关闭时不产生插件通知，符合当前插件范围约束。

## 参数行为

| 类别 | 参数 | Qt 主线行为 |
|---|---|---|
| 帮助 | `--help` | 显示 v8.4.6 参数说明 |
| 实例 | `-multiInst` | 强制新实例 |
| 插件 | `-noPlugin` | 本次启动跳过插件初始化 |
| 会话 | `-nosession` | 不加载且不写回默认会话 |
| 会话 | `-openSession` | 单个路径按 session XML 加载 |
| 配置 | `-settingsDir=` | 配置加载前覆盖用户配置目录 |
| 配置 | `--settings-dir PATH` | 跨平台别名；支持等号形式和 `~/...` 展开 |
| 本地化 | `-LlangCode` | 本次启动临时加载对应 localization XML |
| 文件 | `-r` | 递归展开带 `*`/`?` 的文件参数 |
| 文件 | `-ro` | 命令行文件保持用户只读 |
| 文件 | `-monitor` | 文件改变时自动重载并保持只读 |
| 定位 | `-n`、`-c`、`-p` | 按原版一基行列或 UTF-8 文档位置定位 |
| 语言 | `-lLanguage` | 强制内建 lexer |
| 语言 | `-udl=` | 按名称应用 UDL |
| 窗口 | `-x`、`-y` | 两者均存在时覆盖启动位置 |
| 窗口 | `-notabbar` | 本次启动隐藏主/副视图标签栏 |
| 窗口 | `-alwaysOnTop` | 设置置顶窗口标志 |
| 窗口 | `-systemtray` | 隐藏启动并创建托盘入口 |
| 窗口 | `-titleAdd=` | 追加主窗口标题 |
| 诊断 | `-loadingTime` | 显示启动耗时 |
| 输出 | `-quickPrint`、`/p` | 打印最后打开文件后退出 |
| 输出 | `-export=functionList` | 写入 `<source>.result.json` 后退出 |
| 工作区 | `-openFoldersAsWorkspace` | 将目录参数交给 Folder as Workspace |
| 引用 | `-qn=`、`-qt=`、`-qf=`、`-qSpeed` | 创建引用文档并支持三档逐字输入 |
| 兼容 | `-notepadStyleCmdline` | 拼接剩余参数，必要时追加 `.txt` |
| 插件 | `-pluginMessage=` | 数据模型与单实例 IPC 保留消息 |

## 单实例

- 使用 `QLocalServer`/`QLocalSocket`，服务名由规范化配置目录散列生成。
- 默认单实例模式下，后续进程以紧凑 JSON 转发完整文件级参数并退出。
- `multiInst=2` 时仍探测首实例；后续新实例不恢复共享默认会话，复刻原版避免重复会话的规则。
- `multiInst=1` 与 `-openSession` 组合创建独立会话实例。
- 本地服务使用 `UserAccessOption`，平台 IPC 不进入业务层。

## 关键入口

- `./src/CommandLineOptions.*`
  - 参数解析、路径标准化、递归通配符、本地化映射和 JSON 往返。
- `./src/main.cpp`
  - 启动前覆盖、实例决策、本地 IPC、托盘和耗时提示。
- `./src/MainWindow.*`
  - 会话、文件、位置、语言、只读、监视、打印和退出型参数。
- `./src/Parameters.*`
  - `settingsDir` 与启动本地化覆盖。
- `./src/ScintillaComponent/ScintillaEditView.*`
  - 按原版语言名强制 lexer。

## 验证

- `command-line-options-tests` 覆盖参数全集、递归通配符、Notepad 风格、地区语言映射和 IPC JSON 往返。
- Debug 全量构建成功。
- CTest `19/19` 通过。
- 真实进程验证：
  - `-export=functionList` 退出码 0，生成两个函数条目的 JSON。
  - 第二次启动通过本地 IPC 转发 `-n2 -c3` 并在退出码 0 下返回。

## 边界

- `-pluginMessage=` 只有启用插件系统后才会产生插件通知；当前仅保证解析和传输不丢失。
- Folder as Workspace 当前 UI 只显示一个根目录；传入多个目录时最后一个成为可见根。这是文件浏览器多根能力边界，不是参数丢失。
- `-qn=` 的原版内建彩蛋语料未移植；Qt 版将给定内容作为引用文本处理。
