# DSpellCheck 与 TextFX 可移植功能收口

## DSpellCheck

- `SpellCore` 从单一 Hunspell 实例改为多词典集合；检查时任一启用语言接受单词即
  视为正确，建议按语言合并并去重。
- 词典管理器扫描插件目录、用户配置目录和系统 Hunspell 数据目录，可启用多个语言、
  导入/删除用户管理的 `.dic`/`.aff` 对，并编辑持久用户词典。
- 实现宿主 ABI v1 的可选编辑器上下文菜单导出。右击错误词可选择全部 Hunspell
  建议、忽略当前会话或加入用户词典。
- 旧 `Hunspell/Dictionary` 配置自动迁移到 `Hunspell/ActiveDictionaries`。

## NPPTextFX2

- 命令面扩展到 84 项，补入原版可移植的引号转义/反转义、HTML 表格剥离、文本到
  code command、C/VB 引号感知分行、剪贴板分隔/对齐/重排、前导空白转换、缩进包围、
  UUdecode、括号导航、块复制和标尺插入。
- 纯文本算法保留在 `TextFxCore`，宿主选择区、剪贴板和 Scintilla 操作仍只位于插件
  入口层。
- 不模拟原版 32 位 Viz、Win32 subclass/键盘钩子、外部 Tidy DLL 和原始单字节缓冲区
  转码。这些功能依赖 Qt 版 ABI 中不存在的平台状态，不属于可移植命令集合。

## 验证

- Ubuntu Qt 5 构建通过。
- DSpellCheck：`2/2` CTest 通过，覆盖双语言词典、用户词典和上下文菜单 ABI。
- NPPTextFX2：`2/2` CTest 通过，覆盖新增文本算法和按名称调用的插件命令 ABI。
