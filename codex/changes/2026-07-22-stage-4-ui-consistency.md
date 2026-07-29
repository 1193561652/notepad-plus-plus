# 2026-07-22 阶段四 UI 一致性检验与顶层菜单调整

## 修改范围

- `./src/MainWindow.cpp`
- `codex/analysis/stage-4-ui-consistency-audit.md`

## 代码调整

- 移除 Qt 版非原版顶层 `Format` 菜单。
- 将顶层菜单运行时顺序调整为：
  - File
  - Edit
  - Search
  - View
  - Encoding
  - Language
  - Settings
  - Tools
  - Macro
  - Run
  - Plugins
  - Window
  - ?
- Settings 中的 Import 改为子菜单，更接近原版。
- 新增 Tools 顶层菜单骨架，包含 MD5 与 SHA-256 占位分组。
- Help 顶层标题改为原版风格 `?`。

## 检验结果

详见 `codex/analysis/stage-4-ui-consistency-audit.md`。

## 验证方式

已在 `./build` 执行：

```bash
cmake --build .
```

结果：构建成功。
