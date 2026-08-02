# 2026-07-22 阶段六 复杂兼容功能

## 修改范围

- `./src/ScintillaComponent/FindReplaceDlg.h`
- `./src/ScintillaComponent/FindReplaceDlg.cpp`
- `./src/MainWindow.h`
- `./src/MainWindow.cpp`
- `./src/Parameters.h`
- `./src/Parameters.cpp`
- `./src/WinControls/Preference/PreferenceDlg.h`
- `./src/WinControls/Preference/PreferenceDlg.cpp`

## 行为变化

- `FindReplaceDlg` 新增请求信号，将跨文档和跨文件搜索交给 `MainWindow` 执行。
- Find 页的 `Find All in All Opened Docs` 按钮已启用。
- Find in Files 页的 `Find All` 按钮已接入基础搜索逻辑。
- Search 菜单中的 `Find in Files...` 从占位改为打开 Find in Files 页。
- Search 菜单中的 `Find All in All Opened Documents` 从占位改为执行所有打开文档搜索。
- Find Result 面板支持显示来源文件名，并在激活结果时打开/切换来源文件后定位。
- Find in Files 支持目录、过滤器、递归和隐藏文件选项。
- 所有打开文档搜索按 Buffer 去重，覆盖主/副视图中的已打开文档。
- 所有打开文档搜索结果支持未保存文档：无文件路径时按 Buffer 名称激活对应标签。
- Preferences 的 Dark Mode 页从 stub 改为基础可配置页面。
- `NppGUI` 新增 Qt 扩展字段 `_darkModeEnabled`，通过 `EditorSettings@darkMode` 读写。
- 主窗口支持应用基础 Qt dark palette 和控件样式。

## 验证方式

已在 `./build` 执行：

```bash
cmake --build .
```

结果：构建成功。

## 后续注意

- Find in Files 当前是基础实现，尚未覆盖原版所有边界，例如二进制文件跳过策略、超大文件分批搜索、编码自动检测完整兼容和取消搜索。
- Replace in Files、Replace All in Opened Docs 仍未实现。
- Find in Projects 仍为占位。
- Regex 仍未承诺与原版 Boost.Regex 完全一致。
- Dark Mode 目前是 Qt palette/style 基础实现，尚未完整对齐原版 DarkMode 色彩系统和图标资源。
