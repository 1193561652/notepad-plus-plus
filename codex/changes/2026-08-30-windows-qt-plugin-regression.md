# Windows Qt 插件回归修复

日期：2026-08-30

## 问题

- Windows 上的 `core.autocrlf=true` 会把 v8.4.6 native language XML 检出为
  CRLF，使逐字节 SHA-256 回归失败。
- 简体中文文件关联分类在 v8.4.6 资源中未翻译，UI 回归曾错误期待
  翻译后文本和全角冒号。
- 十个 Qt 移植插件的 CTest `PATH` 直接展开 Windows 分号列表，
  导致 ComparePlus 无法启动 Git、PythonScript 无法加载 Python 运行时。
- MarkdownViewerPlusPlus、NppMarkdownPanel、NPPTextFX2 和 NppFTP 使用了
  Qt 5.14+ API，与项目的 Qt 5.12.12 Windows 基线不兼容。

## 修复

- 将 `installer_common/nativeLang/*.xml` 固定为 LF，并在资源哈希验证时
  规范化 CRLF，保证现有 Windows 工作区也可重现 v8.4.6 基线。
- UI 回归恢复 v8.4.6 文件关联页的真实文本预期。
- 插件 CMake 在写入 CTest `ENVIRONMENT` 前转义 `PATH` 中的所有分号。
- Qt 5.12 使用 `QString::SkipEmptyParts`，更高版本保留
  `Qt::SkipEmptyParts`。
- Markdown 插件在 Qt 5.14+ 继续使用 `QTextDocument::setMarkdown`；
  Qt 5.12 使用内建基础 Markdown 块和行内渲染路径，上层扩展语义保持不变。
- `ui-parity-capture` 在启用跨平台插件系统时也编译对应宿主路径，并提供
  `crossPlatformPlugins` 隔离运行模式，验证加载、菜单注册、安全命令调用、
  同配置重启以及移除一个插件后的再次启动。
- ComparePlus 和 HEX-Editor 的 ABI 回归在 Windows 构建中报告真实 Windows
  宿主信息，避免用 Ubuntu 元数据掩盖平台分支。
- 跨平台插件模块使用 `QLibrary::PreventUnloadHint` 保持到进程退出，防止插件
  注册的 Qt 元类型或进程级状态在 DLL 提前卸载后留下悬空回调；进程结束后
  Windows 仍正常释放模块，插件更新和移除不受影响。

## Windows 验证

- 工具链：Qt 5.12.12 + MinGW 7.3 x64。
- 十个 Qt 插件全部构建成功，CTest `19/19` 通过。
- Notepad++ for Qt 主程序全量 CTest `50/50` 通过，包含简体中文
  亮色/暗色、100%/150% UI 矩阵和 Win32 插件语料。
- 新构建 DLL 已部署到隔离宿主：首次启动与同一配置目录重启均加载
  `10/10`，菜单和安全命令调用通过；暂时移除 HEX-Editor 后再次启动加载
  `9/9`，宿主正常退出，随后恢复隔离产物。
- ComparePlus 在临时 Windows Git 仓库中完成基线提交与 HEAD 比较；
  PythonScript 执行嵌入式 Python/Scintilla 双视图脚本；NppFTP 覆盖 Windows
  临时路径、传输参数、加密凭据的错误口令/篡改拒绝及缓存持久化。
  当前机器未安装 SVN CLI，因此只验证无 SVN 工具时宿主与插件正常降级，
  未声明外部 SVN 成功操作。
