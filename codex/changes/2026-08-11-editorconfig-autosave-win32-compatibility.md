# EditorConfig 与 AutoSave 原版 DLL 兼容

日期：2026-08-11

## 范围

- 使用 v8.4.6 插件清单对应的官方未修改 x64 包：
  - NppEditorConfig 0.4.0，SHA-256
    `ae43152e91d8310ab28859992d3661aba24d8d69a64b93cd12b2f531a1038f56`
  - AutoSave 1.6.1.0，SHA-256
    `572d748449aaea0ce72f2dfa7b36172419386d827b1db1536fc6b57ae23dc104`
- 两个包都通过现有 Plugins Admin/updater 事务安装，不修改 DLL。
- NppEditorConfig 的接口依据 v0.4.0 tag 源码核对；AutoSave 上游不提供源码，
  未反汇编 DLL，只使用公开说明、PE 导入、配置 UI 和受控运行消息确定有限接口。

## 宿主补充

- 编辑器代理新增 EditorConfig 使用的 Tab、缩进、EOL、自动折叠、修改状态等
  标准 Scintilla 消息。
- 主窗口代理新增标准 `WM_COMMAND` 文件命令入口。
- 新增 `NPPM_SAVECURRENTFILEAS`，保持原版
  `BOOL asCopy, const TCHAR *filename` 签名。`asCopy=true` 写出当前 Buffer
  的副本且不改变 Buffer 路径或 clean/dirty 状态；普通模式复用宿主保存和通知流程。
- AutoSave 可在传入的真实主窗口 HWND 上安装自己的 WndProc。没有把 Qt 窗口销毁
  期间的全部系统消息广播给所有插件；该尝试会使现有 17 插件组合在退出时阻塞，
  且本批两个插件都不依赖该广播。

## 已验证行为

- NppEditorConfig：加载、命令表、`.editorconfig` 向上查找、space/tab、
  `indent_size`、`tab_width`、LF EOL、保存前删除尾随空白、补最终换行和 reload。
- AutoSave：加载、命令表、独立 `AutoSave.ini` 读取、Options 对话框、失焦选项、
  主窗口 WndProc 挂接，以及从未保存编辑缓冲区创建带时间戳副本；原文件保持不变。
- Release 全量构建成功，CTest `42/42` 通过。

## 交互验证

- 失焦覆盖保存和分钟定时触发已在真实 Windows 前台桌面会话中验证。
  无桌面测试会话的 `GetForegroundWindow()` 返回空，不能把这种环境限制判为插件失败。
- 可设置 `NPP_QT_TEST_REAL_FOCUS=1` 运行 `win32-configuration-plugins` 进行受控
  前台切换验证；无交互桌面时该模式会明确失败并生成焦点诊断文件。
