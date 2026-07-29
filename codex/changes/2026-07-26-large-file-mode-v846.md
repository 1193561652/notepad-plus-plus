# 2026-07-26 大文件模式方案 A

## 变更

- 固定 200 MiB 阈值并静默进入大文件模式。
- 增加 QsciDocument 文档选项和指针宽度 Scintilla 消息接口。
- 使用 128 KiB 分块加载、增量解码、增量编码和原子保存。
- 修复 UTF-16 高代理项位于块尾时 Qt 5 解码为问号的问题。
- 首次打开、reload、强制编码重解释、Session 备份恢复和 Save As 复用流式路径。
- clone、Document Map、Session 选择区、搜索和替换保留指针宽度位置。
- 关闭 lexer、Wrap、自动完成、括号匹配、Smart Highlight、Function List
  自动分析和周期备份。
- 将 QScintilla/Boost.Regex 静态清单放到唯一 QScintilla 源树旁，避免 qmake
  解析到工作区外同名源码。

## 验证

- `notepadpp-qt` 构建成功。
- `large-file-mode-tests` 通过。
- 完整 CTest：`17/17` 通过。

## 后续

- 方案 B 的提示、状态标记、可调整策略和可取消加载仍为可选优化。
- 32 位拒绝分支尚需在 32 位构建上实机验证。
- 多 GiB 文件压力测试不进入常规测试集。
