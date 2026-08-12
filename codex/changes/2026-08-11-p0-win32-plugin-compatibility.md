# P0 Win32 插件兼容

## 范围

- XMLTools 3.1.1.13 x64
- DoxyIt 0.4.4 x64
- SurroundSelection 1.4.1 x64
- ElasticTabstops 1.3.1 x64

四个插件均使用官方、未修改的发布 ZIP，由项目插件更新器按固定 SHA-256 校验、
安装并生成回执。插件 DLL 本身没有重编译或打补丁。

## 兼容实现

- 补充插件使用的语言类型和 `NPPM_GETLANGUAGENAME`。
- 补充 tab stop、可见行、位置、Indicator 和 Annotation 等同步 `SCI_*` 消息。
- `SCI_GETDIRECTFUNCTION` 返回宿主 thunk，使旧插件仍通过原版 direct function/
  direct pointer 形式调用编辑器。
- 区分两种 Scintilla ABI：DoxyIt/ElasticTabstops 的旧 direct 接口使用
  `Sci_PositionCR=long`，XMLTools 的 HWND 消息使用 `Sci_PositionCR=intptr_t`。
  旧结构只在 direct thunk 内转换，不能对所有 HWND 消息统一转换。
- Qt QAction 在插件命令返回后仅同步插件实际修改的 `_init2Check`，保持原版
  `CheckMenuItem`/`NPPM_SETMENUITEMCHECK` 的勾选语义。

## 验证

- DoxyIt：文件注释、函数参数注释、Settings 原生窗口及 owner。
- ElasticTabstops：tab stop 计算、tab 转空格。
- SurroundSelection：Enable 命令及勾选状态。
- XMLTools：格式化、线性化、转义、反转义、注释、取消注释、畸形 XML 注解、
  标签自动闭合、Prevent XXE 状态和 Options 窗口。
- 四插件组合加载、命令注册、正常退出。
- 2026-08-11 Release 全目标构建成功，CTest `41/41` 通过。

## 崩溃诊断

曾观察到一次 `ui-parity-capture.exe` 访问冲突，地址落在 Scintilla
`Editor::WndProc`。根因是把旧插件的 32 位 `Sci_CharacterRange` 原样传给
Scintilla 5 的指针宽度结构。后续几次现象没有 Application Error/WER 记录，实际是
测试线程等待不存在的 XMLTools 对话框后被超时终止；Annotation 模式不会打开该
对话框。修正 ABI 边界和测试等待后，P0 测试约 1.2 秒正常完成。
