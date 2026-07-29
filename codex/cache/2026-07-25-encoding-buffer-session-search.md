# 本地缓存：四项基础兼容性修复

缓存日期：2026-07-25

## 当前结论

- 编码/EOL、Buffer/双视图、会话、搜索替换的高风险基础差异已完成一轮代码修复。
- 编解码事实源是 `TextFileCodec`；文件搜索替换不得再自建另一套 BOM/EOL 检测。
- Buffer 与 view 是一对多关系；关闭或移动标签必须先区分“释放一个 view”和“销毁最后一个 Buffer”。
- session 时间戳必须按 Windows FILETIME 两个 32 位字段读写，不能写 Unix 秒冒充原字段。
- Scintilla 位置是 UTF-8 字节偏移；QString/QRegularExpression 位置是 UTF-16 字符索引。
- 搜索零长度匹配在替换后必须跨过原始字符边界，不能依赖普通 `findNext()`。

## 最近验证

- `cmake --build build --parallel 4`：通过。
- `ctest --test-dir build --output-on-failure`：1/1 通过。

## 继续工作前优先阅读

- `codex/changes/2026-07-25-core-compatibility-fixes.md`
- `codex/analysis/2026-07-25-core-compatibility-status.md`
- `codex/modules/notepad-plus-plus/file-buffer.md`
- `codex/features/search-replace.md`
