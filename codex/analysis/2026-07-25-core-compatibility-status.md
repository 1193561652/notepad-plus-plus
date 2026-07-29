# 2026-07-25 四项基础能力差异状态

| 范围 | 当前状态 | 本次已对齐 | 仍有差异 |
|---|---|---|---|
| 编码与 EOL | 基础行为完成 | BOM、UTF-8/16、代码页保存校验、按代码页重解释、EOL 检测/转换、原子保存 | uchardet/cookie、大文件和全部异常策略 |
| Buffer 与双视图 | 基础行为完成 | 一个 Buffer 多 view、共享 QsciDocument、move/clone/close 生命周期、状态同步 | 原版 Document 引用计数实现不同；复杂拖放仍需 UI 验证 |
| 会话 | 核心字段完成 | 主副视图、clone、活动索引、选择/滚动/书签/折叠、备份、FILETIME、只读 | Document Map 状态、标签颜色和异常 session 语料仍需验证 |
| 搜索替换 | 常用范围完成 | Normal/Extended/Regex、选区、打开文档、文件、编码保持、零长度前进 | Boost.Regex 全语法、结果 lexer、取消/进度和大目录性能 |

## 关键实现入口

- 编解码：`TextFileCodec::decode()`、`decodeAs()`、`encode()`。
- 保存：`FileManager::saveBuffer()`、`saveBufferCopy()`。
- 多视图：`Buffer::addView()`、`removeView()`、`DocTabView::addBufferView()`、
  `MainWindow::moveToOtherView()`、`cloneToOtherView()`。
- 会话：`MainWindow::saveSession()`、`restoreSession()`、
  `NppParameters::writeSessionXml()`。
- 搜索：`FindReplaceDlg::onReplace()`、`onReplaceAll()`、
  `MainWindow::onFindAllOpenedDocsRequested()`、
  `onReplaceAllOpenedDocsRequested()`、`onReplaceInFilesRequested()`。

## 后续验证重点

1. 用原版生成的多视图 `session.xml` 做往返和 UI 恢复测试。
2. 用 Shift-JIS、GB18030、Big5 和 Windows-125x 真实语料验证重新解释和保存。
3. 建立 Boost.Regex 与 Qt Regex 的差异用例清单。
4. 手工验证 clone 后编辑、保存、关闭任一标签、move 和应用重启恢复。

## 2026-07-25 后续增量

- 编码检测已补入 XML/HTML 声明、uchardet、`openAnsiAsUTF8`、
  `DetectEncoding`、UTF-8 cookie 和 session 代码页语义。
- Session 的标签颜色、Document Map 状态和文件浏览器选中项已可应用到 UI。
- “uchardet/cookie”和“少量 Session UI 专属状态”不再列为本轮已知差异。
- 大文件模式与 Boost.Regex 全语义已明确记录为待决代办，不属于本轮完成范围。
