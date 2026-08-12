# NppEditorConfig 与 AutoSave 兼容结论

日期：2026-08-11

## 结论

- NppEditorConfig 0.4.0：从 `U` 复评为有限兼容 `A`。对应 tag 源码可核对，
  无 Dock、线程、Hook 或 Scintilla direct pointer 依赖，核心编辑和保存行为已自动验证。
- AutoSave 1.6.1.0：从 `U` 复评为有限兼容 `B`。官方 DLL 可稳定加载、配置、
  安装 WndProc 并创建正确副本；上游无源码且真实失焦/分钟定时依赖交互桌面，
  因而暂不标为 `A`。

## 精确宿主表面

- EditorConfig 主窗口：当前完整路径、当前 Scintilla、文件名、设置语言、执行菜单命令。
- EditorConfig 通知：`NPPN_BUFFERACTIVATED`、`NPPN_FILEBEFORESAVE`。
- EditorConfig 编辑器：Tab/缩进/EOL、自动折叠、行与字符访问、追加和删除范围。
- AutoSave 主窗口：插件配置目录、标准文件保存命令、`NPPM_SAVECURRENTFILEAS`。
- AutoSave 平台行为：在宿主真实主 HWND 上使用 `SetWindowLongPtrW` 安装 WndProc，
  使用 Win32 Timer 和对话框资源。

## 风险边界

- 不把 Qt 窗口销毁期全部系统消息广播给插件。实测该方案使既有 17 插件组合退出阻塞；
  这不是本批插件的必要依赖。
- AutoSave 的 DLL 内部 Timer/焦点竞争没有源码可审计，需保留真实桌面和正常退出验证。
- 非 Windows 不加载这两个原版 DLL；后续跨平台需求应按既定决策优先提供独立 Qt 插件，
  不把插件业务逻辑直接并入主程序。

