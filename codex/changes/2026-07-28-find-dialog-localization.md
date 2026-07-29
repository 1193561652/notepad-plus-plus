# Find 对话框本地化修复

## 问题

`applyNativeLang()` 在切换或首次应用语言时会销毁未显示的
`FindReplaceDlg`。Find 和 Replace 入口会在重建后应用语言资源，但 Mark、
Find Next/Previous、Find in Files、Find All 和 Project Panel 入口只重建
对话框，导致其控件保留英文。

Find in Projects 页另有 6 个可见控件缺少 `objectName` 和中文资源，即使正常
执行本地化也无法翻译。

## 修改

- 新增 `MainWindow::ensureFindReplaceDialog()`，集中执行延迟创建、信号连接和
  `NativeLangSpeaker::changeDlgLang()`。
- 所有 Find 对话框入口统一使用该函数。
- 为 Find in Projects 页的项目面板复选框、查找、替换和关闭按钮补齐稳定
  `objectName`。
- 补齐简体中文语言资源。
- 新增 `find-dialog-localization-tests`，检查入口收口、控件标识和 XML 覆盖。

## 验证

- `cmake --build build --target notepadpp-qt -j 4`：通过。
- `ctest --test-dir build --output-on-failure`：26/26 通过。
- 本次未执行可见 GUI 人工检查；已运行的旧进程需要重新启动后才会加载新构建。
