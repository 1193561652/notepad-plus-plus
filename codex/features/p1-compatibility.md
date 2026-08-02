# P1 功能兼容

## 范围

本轮 P1 覆盖 Finder、标记文本复制、外部文件变化、打印渲染、高级排序、
列编辑、UDL 2.1 和保存搜索宏。插件系统不在本轮范围。

## Finder 与标记

- Copy Marked Text 从查找标记指示器 31 提取所有区间，按原文档行尾连接。
- Find Result 按搜索、来源文件和结果行形成三级语义结构。
- 结果保存文档及行内 UTF-8 字节位置，匹配片段使用当前调色板高亮。
- Find in Files 和 Find in Projects 提供进度和取消。

## 外部文件变化

- 同时监听文件和父目录，并以 2.5 秒轮询作为网络路径 watcher 的兜底。
- 支持内容变化、删除、权限变化和同目录唯一候选重命名识别。
- watcher 回调使用 Buffer 快照，避免提示过程中关闭文档使遍历失效。

## 打印

- `ScintillaComponent/Printer` 在页面正文外渲染左右中三段页眉页脚。
- 支持路径、文件名、日期、时间和当前打印页等 v8.4.6 变量。
- 打印颜色模式使用 Scintilla 配置；行号按配置临时控制 margin 0。

## 高级编辑

- 排序支持大小写、整数、小数点、小数逗号、随机、反转和矩形列范围。
- Column Editor 支持文本/数字、初值、增量、重复、四种进制及前导零。
- 新动作注册 v8.4.6 command ID，可由 shortcuts.xml 和宏 type 2 触发。

## UDL

- 使用 v8.4.6 原版 `LexUser.cxx`，编入独立 `npp-lexilla` 静态库。
- 当时的编辑器包装扩展到 31 个关键词集合，并补入 LexUser 所需 lexlib API；
  当前实现已迁移到独立 Lexilla 5.1.9。
- 解析 28 个关键词列表、8 组前缀、折叠、样式和 nesting。
- 同时加载根 `userDefineLang.xml` 与 `userDefineLangs/*.xml`。
- 设计器写回语言原始来源文件；保留 XML 原始 styleID，不升级结构。
- 关键词集合按原版 mapper 映射，支持带引号的多词关键字。

## 宏

- shortcuts.xml Action type 3 支持保存搜索状态及执行命令。
- 可回放当前文档、打开文档、文件和 Project Panel 范围的查找替换。

## 自动验证

- Debug 主程序和全部测试目标构建成功。
- CTest 21/21 通过。
- 覆盖 Copy Marked Text、LexUser 关键词样式和 Finder UTF-8 行内字节位置。
- `npp-command-registry-tests` 检查命令对象名/ID 唯一性、动作存在性和关键
  v8.4.6 ID。
- `file-association-model-tests` 检查原版 10 组扩展名分类、去重和自定义扩展名
  规范化。

运行程序和人工交互验证见
`codex/validation/2026-07-28-p1-pending/README.md`，等待用户授权。

## 命令 ID 与文件关联

- `NppCommandRegistry` 集中保存 QAction 对象名到 v8.4.6 command ID 的映射，
  `MainWindow` 不再维护内嵌散列表。
- 已修正 Word Wrap、Restore Zoom 和 Reverse Line Order 的错误/失效映射，
  并补齐自动完成、标记跳转、视图面板、折叠、EOL、设置、Hash、Run 和
  Window 等已实现动作。
- Windows 文件关联页恢复原版分类、可关联扩展、已关联扩展、自定义扩展、
  添加/移除和管理员状态。
- Windows 注册表实现沿用 `Notepad++_file` 与 `Notepad++_backup`，移除关联时
  恢复原默认 ProgID；平台代码位于 `PlatformServices`。
- 非 Windows 不模拟注册表语义，保留系统默认应用设置入口。
