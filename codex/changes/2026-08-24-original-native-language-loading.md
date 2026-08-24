# 原版 native language 加载机制对齐

## 原版结构

- 官方语言源文件位于 `PowerEditor/installer/nativeLang/*.xml`。
- 安装程序把候选语言包安装到 `localization/`。
- 当前语言是用户目录或程序目录中的完整 `nativeLang.xml`。
- `NativeLangSpeaker` 按菜单命令 ID、对话框控件 ID 和
  `MiscStrings` 键加载文本。

## Qt 平台适配

- 保留 `NppParameters` 选择完整语言包和 `NativeLangSpeaker` 管理文本
  的原版职责边界。
- 数字菜单 ID 通过 `NppCommandRegistry` 对应 `QAction`。
- 原版对话框 ID 在 `localization.cpp` 的平台层对应 Qt objectName；
  XML 不保存 Qt 专用字段。
- Qt 控件文本的通用回退由官方 `english.xml` 与目标语言文件
  的同 ID/同节点配对提供，不使用 `QTranslator`。

## 资源约束

- `resources/nativeLang/` 中的 94 份 XML 必须与 v8.4.6 官方目录
  逐字节一致；它们是打包输入，构建后复制到程序目录 `localization/`。
- 运行时扫描程序目录 `localization/`，不把语言文件嵌入 qrc。
- 不保留项目内部 `resources/localization/` 副本。
- 不引入 `.ts/.qm` 或 `QTranslator` 语言切换链路。

## 验证

- 资源 SHA-256 回归测试固定三份代表性官方文件，并验证安装目录可发现
  全部 94 种语言。
- 语言 XML 回归测试拒绝任何 `objectName` 属性。
- 简体中文和日语 UI 运行测试覆盖菜单、查找对话框和首选项。
