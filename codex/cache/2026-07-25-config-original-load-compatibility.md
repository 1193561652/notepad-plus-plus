# 配置兼容修复缓存

- 原版加载错误根因：Qt 写出的短格式 `nativeLang.xml`。
- 正确行为：`nativeLang.xml` 必须是完整且与实际安装原版匹配的本地化文件，
  不是语言代码标记。
- 2026-08-24 后不再分离 Qt UI 映射资源；
  `installer_common/nativeLang/*.xml` 的 94 份文件与 v8.4.6 原版保持一致，
  构建后复制到程序目录 `localization/`，不嵌入 qrc。
- Qt 控件适配由 `src/localization.cpp` 使用原版数字 ID 完成。
- Qt 私有状态：`qtState.ini`。
- 原版共享文件中禁止出现：`EditorFont`、`EditorSettings`、`WindowState`。
- 已验证未知配置节点和属性往返保留。
- 当前机器安装原版：Notepad++ v7.81；随附中文包版本：7.7.2。
