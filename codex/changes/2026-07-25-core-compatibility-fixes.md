# 2026-07-25 编码、Buffer、会话与搜索替换兼容性修复

## 修改范围

- `./src/MISC/TextFileCodec.*`
- `./src/MISC/FileManager.*`
- `./src/ScintillaComponent/Buffer.*`
- `./src/WinControls/TabBar/DocTabView.*`
- `./src/ScintillaComponent/FindReplaceDlg.*`
- `./src/MainWindow.*`
- `./src/Parameters.*`
- `./tests/CoreBehaviorTests.cpp`

## 行为变化

### 编码与 EOL

- 文件解码集中到 `TextFileCodec`，统一处理 UTF-8/UTF-16 BOM、无 BOM UTF-16、
  UTF-8 校验、回退代码页、二进制判断和解码后的 EOL 检测。
- 文件保存使用 `QSaveFile` 原子提交；目标代码页无法表示文本时返回错误。
- 编码和 BOM 作为 Buffer 元数据参与脏状态。
- 字符集菜单通过 `TextFileCodec::decodeAs()` 按指定代码页重新解释磁盘字节；
  “Convert to”只改变下次保存使用的编码。
- EOL 转换作用于共享文档，并同步所有关联视图的 EOL mode。

### Buffer 与双视图

- `Buffer` 维护所有 `ScintillaEditView`，不再依赖单个 view 指针和外部 clone 映射。
- 克隆视图通过 `QsciDocument` 共享内容和撤销历史。
- 关闭克隆标签只释放该视图；最后一个视图关闭时才询问保存并销毁 Buffer。
- 跨视图移动搬运当前 editor widget；目标视图已有同一 Buffer 时关闭重复视图。
- 所有视图保存点、只读状态、lexer 和 EOL 状态在保存/reload 后同步。

### 会话

- 活动索引按实际写入 session 的文件列表计算。
- 保存和恢复主/副视图、共享文档 clone、选择模式、选择范围、滚动位置、书签、
  折叠、只读状态、编码、备份路径和文件浏览器根目录。
- `originalFileLastModifTimestamp` 与 `High` 按原版 Windows FILETIME 低/高 32 位写入；
  读取仍兼容早期 Qt 版写入的 Unix 秒。
- `writeSessionXml()` 返回 TinyXml 的真实保存结果。
- 仅快照模式允许依赖备份无提示退出；关闭快照后退出会询问脏文档。

### 搜索替换

- Extended 模式同时转义查找和替换文本。
- Regex 的 “`.` 匹配换行”在当前文档、打开文档和文件搜索中统一传递。
- 单次 Replace 可替换当前正则匹配选区。
- Replace All 始终从文档开头开始；选区替换会随文本长度变化调整边界。
- 零长度正则按原始字符边界前进，避免无限替换，并支持打开文档范围。
- 打开文档操作按 Buffer 去重，跳过只读/二进制文档并保留选择。
- 文件搜索使用统一编码解码；文件替换保持原编码和 BOM，并用原子写入。
- Find All 结果把 QString 字符位置转换为 Scintilla UTF-8 字节位置。

## 验证

```powershell
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

- 应用目标构建通过。
- `core-behavior-tests` 通过，覆盖 BOM、UTF-16、无 BOM UTF-16、代码页回退、
  显式重新解释、不可表示字符拒绝、二进制检测和 Buffer 多视图/元数据脏状态。

## 仍保留的差异

- 无 BOM 非 UTF-8 编码仍依赖 locale fallback，尚未接入原版 uchardet/cookie 全链路。
- Regex 使用 QRegularExpression/QScintilla，不承诺与原版 Boost.Regex 的全部语法、
  替换表达式和错误细节一致。
- 原版大文件模式、网络路径、符号链接、权限变化和所有文件冲突策略仍需专项验证。
- Session 的 Document Map 专属状态和标签颜色字段可读写，但当前 UI 尚不能完整应用。
