# 配置兼容修复缓存

- 原版加载错误根因：Qt 写出的短格式 `nativeLang.xml`。
- 正确行为：`nativeLang.xml` 必须是完整且与实际安装原版匹配的本地化文件，
  不是语言代码标记。
- Qt UI 映射资源：`resources/nativeLang/chineseSimplified.xml`。
- 原版兼容复制资源：`resources/localization/chineseSimplified.xml`。
- Qt 私有状态：`qtState.ini`。
- 原版共享文件中禁止出现：`EditorFont`、`EditorSettings`、`WindowState`。
- 已验证未知配置节点和属性往返保留。
- 当前机器安装原版：Notepad++ v7.81；随附中文包版本：7.7.2。
