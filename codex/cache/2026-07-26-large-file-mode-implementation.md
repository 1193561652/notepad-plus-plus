# 本地缓存：大文件模式方案 A 已完成

- 固定阈值：200 MiB，达到阈值静默降级。
- Scintilla：`STYLES_NONE | TEXT_LARGE`。
- I/O：128 KiB 流式加载和保存，不生成全文 QString 副本。
- 编码：UTF-8、UTF-16LE/BE、旧代码页和跨块代理项已测试。
- 状态：打开、reload、重解释编码、Session、clone、Save/Save As 一致。
- 降级：lexer、Wrap、自动完成、匹配、Smart Highlight、Function List、周期备份。
- 超大文件：原版公式；32 位拒绝、64 位确认；核心位置使用 `qintptr`。
- 构建：QScintilla 静态清单位于其源码目录，启用 `SCI_OWNREGEX`。
- 验证：主程序构建成功，CTest 17/17。
- 后续仅保留方案 B 可选优化及多 GiB/32 位实机压力验证。
