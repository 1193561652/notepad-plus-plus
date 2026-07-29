# 本地缓存：P0 配置与 UI 验证

日期：2026-07-26

- 配置未知节点/属性保持已由真实写入入口和 14 组语料覆盖。
- config、session、shortcuts 的写回不得重建受管元素之外的 XML 结构。
- 原版 v8.4.6 已成功读取 Qt 写回后的隔离配置。
- UI 基线目录：`./build/p0-ui-audit/`。
- Qt UI 可重复采集入口：CMake 目标 `ui-parity-capture`。
- Windows UI 字体统一入口：`src/MISC/UiFont.h`。
- Qt 版当前有 5 个查找页和 19 个首选项页。
- 工具栏数量、Preferences General 分组/按钮和 Find 精确间距仍是已知差异。

