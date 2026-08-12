# EditorConfig 与 AutoSave 代码索引

## 安装语料

- `third_party/win32-plugins/NppEditorConfig/0.4.0/x64/NppEditorConfig-040-x64.zip`
- `third_party/win32-plugins/AutoSave/1.6.1.0/x64/AutoSave_dll_1v61_x64.zip`
- `tests/Win32PluginCorpusInstallPlan.json.in`：测试安装事务。
- `CMakeLists.txt`：归档校验、测试夹具、`win32-configuration-plugins`。

## 宿主入口

- `src/Win32PluginSystem/Win32PluginInterface.h`：
  `NppMessageSaveCurrentFileAs` 和 v8.4.6 ABI 常量。
- `src/Win32PluginSystem/Win32MainWindowAdapter.cpp`：
  `WM_COMMAND`、`NPPM_SAVECURRENTFILEAS`、路径/语言/菜单命令代理。
- `src/Win32PluginSystem/Win32EditorMessageAdapter.cpp`：
  EditorConfig 使用的标准 Scintilla 消息代理。
- `src/NppIO.cpp`：`MainWindow::saveCurrentFileAsForPlugin()`；
  `asCopy=true` 进入 `FileManager::saveBufferCopy()`，否则进入正常 `doSave()`。
- `tests/UiParityCapture.cpp`：`configurationPlugins` 真实 DLL 功能矩阵。
- `tests/Win32FocusWindow.cpp`：有交互桌面时的失焦测试辅助程序。

## 更新规则

修改上述消息、保存、通知、Buffer 或插件安装逻辑后，至少执行：

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release -R "^win32-configuration-plugins$" --output-on-failure
ctest --test-dir build -C Release --output-on-failure
```

真实失焦验证使用 `NPP_QT_TEST_REAL_FOCUS=1`，仅在可取得 Windows 前台窗口的
交互会话执行。计时器验证使用 `NPP_QT_TEST_AUTOSAVE_TIMER=1`，将周期设为最小的
1 分钟，并断言磁盘内容和编辑器 dirty 状态。
