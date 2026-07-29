# 编码检测与 Session UI 状态补全

日期：2026-07-25

## 编码检测

- 将原版 `PowerEditor/src/uchardet/` 纳入 Qt 主线构建。
- 新增 `MISC/EncodingMapper.*`，负责 Notepad++ session 代码页与 Qt codec 名称互转。
- `TextFileCodec` 的打开优先级对齐为：BOM、无 BOM UTF-16 特征、
  XML/HTML 编码声明、有效 UTF-8、uchardet、locale fallback。
- `openAnsiAsUTF8` 和 `DetectEncoding` 配置参与检测。
- 保留原版对 uchardet `TIS-620` 高误报结果的禁用策略。
- `uniUTF8` 恢复为 UTF-8 BOM，`uniCookie` 恢复为 UTF-8 无 BOM 的原版含义。
- 快照备份按 Buffer 外部编码和 BOM 写入；字符不可表示时使用带 BOM UTF-8。

## Session UI 专属状态

- Session 的 `encoding` 字段保存外部代码页，不再误用 `UniMode` 枚举。
- 恢复 session 和快照备份时按 session 代码页解码，Unicode BOM 始终优先。
- 标签颜色 `tabColourId` 已进入 Buffer 并由 TabBar 绘制。
- Document Map 字段已在 Buffer、session XML 和面板之间采集/恢复。
- 文件浏览器的根目录及 `latestSelectedItem` 均可恢复。

## 关键入口

- 编码：`TextFileCodec::decode()`、`decodeAs()`、`EncodingMapper`
- 配置：`NppParameters::feedGUIConfig()`、`writeConfigXml()`
- 会话：`MainWindow::saveSession()`、`restoreSession()`
- UI：`DocTabView::setIndividualTabColour()`、
  `DocumentMapPanel::sessionState()`、`restoreSessionState()`、
  `FileBrowserPanel::setSelectedPath()`

## 验证

- `cmake --build build --parallel 4` 通过。
- `ctest --test-dir build --output-on-failure` 通过。
- 回归测试覆盖 BOM/cookie、XML 声明、uchardet Windows-1251 检测、
  代码页映射和 Buffer Session UI 元数据。

## 明确未实现

- 大文件模式：已记录为待决代办。
- Boost.Regex 全语义：已记录为待决代办。
