# DSpellCheck-qt Lexer 与右键建议收口

日期：2026-08-14

## 原版结构

- `src/npp/ScintillaUtils.cpp` 按 Lexer/style 将文本分为正文、注释、字符串、标识符和
  未知类别。
- `src/core/SpellCheckerHelpers.cpp` 根据设置决定哪些类别进入拼写检查。
- `src/ui/ContextMenuHandler.cpp` 识别右击位置的错误词，生成建议、忽略和加入词典命令，
  并由插件执行修改。

## Qt 实现

- Qt 目标直接编译原版 `ScintillaUtils` 完整分类表，仅通过 `QtStyleSettings` 适配四个
  平台无关设置字段，没有复制第二套 Lexer 映射。
- 增量检查结果通过 `SCI_GETLEXER` 和 `SCI_GETSTYLEAT` 分类；语言切换会清空缓存并重检。
- 设置对话框和 `DSpellCheck.ini` 已接入注释、字符串、标识符及默认 UDL 样式选项。
- 跨平台 ABI v1 增加两个可选上下文菜单导出。宿主传入视图和点击字节位置，复制菜单
  文本并回调命令；DSpellCheck 自己生成 Hunspell 建议并执行替换、忽略或加入词典。
- 没有可用词典时不产生错误标记和建议菜单，保持原版无工作 speller 时的降级行为。

## 验证

- DSpellCheck 核心测试覆盖 C++ 注释、字符串、标识符、YAML 正文和未知样式分类。
- ABI 烟雾测试创建临时 Hunspell 词典，验证 `helo` 的右键建议和替换为 `hello`。
- 宿主跨平台插件测试覆盖可选符号发现、菜单项复制和命令 ID 回调。
