# 大文件模式采用 Notepad++ v8.4.6 原版行为

日期：2026-07-26

## 背景

Qt 主线尚未实现原版大文件模式。当前文件打开路径使用
`QFile::readAll() -> QByteArray -> QString -> QsciScintilla::setText()`，
不能视为与原版分块加载等价。

本决策只确定目标行为和实现边界，不表示功能已经完成。

## 结论

第一版采用方案 A，完整复刻 Notepad++ v8.4.6 的大文件模式：

- 大文件阈值固定为 `200 * 1024 * 1024` 字节。
- 当文件大小大于或等于阈值时，静默进入大文件模式。
- 不增加 200 MiB 提示、状态栏标记或可配置阈值。
- 使用 `SC_DOCUMENTOPTION_STYLES_NONE |
  SC_DOCUMENTOPTION_TEXT_LARGE` 创建 Scintilla 文档。
- 保持原版约 2 GiB 文件的架构分支和提示语义。
- 降级功能、菜单可用性及副作用以 v8.4.6 源码行为为准，不主动优化。

方案 A 的实现必须同时覆盖原版的分块加载模型。只增加
`Buffer::isLargeFile()` 或关闭 UI 功能，但仍使用 Qt 全文副本加载，不算完成。

## 原版阈值与 UI 行为

### 200 MiB

- 判定条件：`fileSize >= NPP_STYLING_FILESIZE_LIMIT`。
- `NPP_STYLING_FILESIZE_LIMIT` 为 `200 * 1024 * 1024`。
- 不弹出确认框。
- 若 Word Wrap 已开启，按原版命令路径关闭。

### 约 2 GiB

原版判断的不是简单的 `fileSize >= 2 GiB`，而是：

```text
fileSize + min(1 MiB, fileSize / 6) > INT_MAX
```

- 32 位进程拒绝打开，并显示文件过大提示。
- 64 位进程显示 `WantToOpenHugeFile` 是/否确认框。
- 用户拒绝后终止打开。

Qt 移植实现时必须先核验 QScintilla 消息参数、返回值和所有 Scintilla position
在目标编译器上的宽度。不能仅依据 `sizeof(void*) == 8` 就宣称支持 2 GiB 以上文件。

## 原版降级策略

大文件 Buffer 必须按 v8.4.6 行为处理：

- 文档使用 `SC_DOCUMENTOPTION_STYLES_NONE`，不分配逐字符样式。
- 文档使用 `SC_DOCUMENTOPTION_TEXT_LARGE`。
- Buffer 语言保持为普通文本，不按扩展名或文件头启用语言。
- 关闭 Word Wrap。
- 不执行自动完成和配对字符自动插入路径。
- 不执行括号匹配、XML 标签匹配、Smart Highlight 和 URL Hotspot。
- 不执行 Session 周期快照备份。
- 不执行保存时备份。
- 查找、替换、普通编辑、撤销和手动保存不因大文件模式被额外关闭。

若 Qt 版尚未实现某项原版功能，例如保存时备份，则只需保证未来实现该功能时
仍检查大文件状态；不能把“功能本身尚未实现”记作大文件限制已经验证。

## 实现边界

后续代码实现至少需要覆盖：

1. 文件大小预判和 `Buffer` 大文件状态。
2. QScintilla 文档创建选项及 `QsciDocument` 生命周期安全。
3. 分块读取、增量编码识别/转换和 `SCI_APPENDTEXT`。
4. 分块保存，避免 `view->text()` 和完整输出 `QByteArray`。
5. Word Wrap、lexer、自动完成、Smart Highlight、匹配和备份路径守卫。
6. reload、Session 恢复、双视图 clone 和 Save As 的大文件状态一致性。
7. 32/64 位约 2 GiB 分支。
8. 分配失败、读取失败和编码转换失败的原版对应提示。

## 方案 A 已知缺陷

- 200 MiB 时静默降级，用户可能不知道语法高亮和自动备份已经关闭。
- 自动备份静默关闭可能造成数据保护预期落差。
- 固定阈值不能根据设备内存和平台能力调整。
- 原版 Word Wrap 命令路径可能产生超出当前 Buffer 的可见副作用。
- 同步加载期间界面可能长时间无响应。
- 原版 2 GiB 确认只说明耗时，不能保证内存一定足够。
- Scintilla 大文档模式仍是完整文档模型，不是分页查看器。

这些缺陷是本轮为保持 v8.4.6 行为一致性而接受的，不应在方案 A 实现中自行修正。

## 后续可选优化：方案 B

方案 B 保留 200 MiB 阈值和核心降级策略，但可以在后续独立决策中选择：

- 打开后显示一次非模态大文件模式提示。
- 在状态栏或标签页提供持续的大文件模式标记。
- 明确列出已关闭的高亮、自动完成、Word Wrap 和备份功能。
- 将 Word Wrap 降级改为每个 Buffer 独立处理。
- 为长时间加载增加进度和取消。
- 在支持前完成 2 GiB 以上位置类型与内存安全审计。

方案 B 当前仅记录为可选优化，不进入方案 A 的实现和验收范围。

## 源码依据

- `v8.4.6:PowerEditor/src/ScintillaComponent/Buffer.h`
- `v8.4.6:PowerEditor/src/ScintillaComponent/Buffer.cpp`
- `v8.4.6:PowerEditor/src/NppNotification.cpp`
- `v8.4.6:PowerEditor/src/Notepad_plus.cpp`
- `v8.4.6:PowerEditor/src/NppIO.cpp`
- `v8.4.6:scintilla/doc/ScintillaDoc.html`

## 实施结果

2026-07-26 已按本决策完成方案 A。实现包含固定阈值、Scintilla 大文本/无样式
文档、128 KiB 流式加载和保存、UTF-16 跨块保护、约 2 GiB 分支、功能降级、
reload、强制编码重解释、Session、双视图和指针宽度搜索替换。

验证结果为主程序构建成功、CTest `17/17` 通过。方案 B 仍保持为后续独立
可选优化，不属于当前实现。
