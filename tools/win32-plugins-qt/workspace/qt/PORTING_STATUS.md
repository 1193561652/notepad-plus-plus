# win32-plugins Qt 移植记录

要求：以目录中的原版源码为逻辑基线，保留独立插件边界、菜单、配置和通知流程。
构建入口位于 `../CMakeLists.txt`；Qt 插件使用宿主的跨平台 C ABI，不加载 Win32 DLL。

## 用户确认跳过：缺少原始源码，无法按要求移植

2026-09-05 用户确认没有其他源码路径，要求跳过并记录。
`../NppPlugins/README.md` 的 Source code 段明确说明源码未公开；现有目录只有发布包。

| 插件 | 状态 | 原因 |
| --- | --- | --- |
| AutoSave | 无法移植，跳过 | 仅有 DLL 发布包，缺少原始逻辑源码 |
| LanguageHelp | 无法移植，跳过 | 同上 |
| MenuIcons | 无法移植，跳过 | 同上 |
| OpenSelection | 无法移植，跳过 | 同上 |
| RunMe | 无法移植，跳过 | 同上 |
| TakeNotes | 无法移植，跳过 | 同上 |
| TopMost | 无法移植，跳过 | 同上 |

这些插件没有创建占位 Qt 实现，也不计入移植完成数量。

## 已实现并验证的批次

| 插件 | 原始逻辑与 Qt 替换边界 | 验证 |
| --- | --- | --- |
| qkNppReverseLines | 原 reverseLines 共享头；替换选区消息和 About | ABI、CR/LF/CRLF、NUL、空选区、文档光标 |
| SelectQuotedText | 原 CheckInString 全表和 SelectQuotedText 共享头；NppMessenger 适配 | ABI、字符串范围、词回退、快捷键 |
| MIME Tools | 直接编译原 b64/qp/url/saml/tinflate；Qt 命令适配 | ABI、Base64/URL/QP/SAML、二进制、多选区拒绝 |
| nppAutoDetectIndent | 原统计与权重/应用算法共享头，原 Settings.cpp；MyPlugin 保留缓存及生命周期 | ABI、空格/Tab、禁用恢复、关闭、另存为缓存 |
| SelectToClipboard | 原 SelPos、doCopySelection、CopyRoutine 范围合并与触发顺序；QSettings/QClipboard | ABI、配置持久化、换标签抑制、矩形剪贴板 |
| BracketsCheck | 原 C# 栈算法按 UTF-16 字符移植，保留四类括号开关及出错位置规则 | ABI、真实弹窗、Tab/中文/选区位置 |
| Remove_dup_lines | 原 C# StringReader/Distinct 顺序及空白行标识逻辑 | ABI、真实编辑器去重、空白行保留 |
| SurroundSelection | 原 SurroundSelectionsWith 共享头；Qt 编辑器键盘事件替换 Win32 hook | 多选区、内层选区恢复、单步撤销、关闭开关、不影响其他输入框 |
| SecurePad | 直接编译原 Blowfish；保留零填充、ECB、大写十六进制和密钥窄化 | 原始独立加密向量、文档/选区往返、真实密码框、畸形密文拒绝 |
| ElasticTabstops | 直接编译原单元格测量、列宽扩展、增量计算与转空格；原配置解析提为共享核心 | 相邻行与分隔块、缩放、增量编辑、禁用清理、转空格及撤销、配置往返 |
| Merge-files-in-one | 原 C# 行配对、顺序切换、TrimEnd 和 .NET replacement token 语义 | 双文档快照、反向合并、源文档不变 |
| nppURLPlugin | 直接复用 URL 编解码器、原 Split 和 Profile 配置 | 新文档/原文档输出、换行、工具栏及撤销 |
| BetterMultiSelection | 原选区排序、偏移修正、键盘分派和多行粘贴共享头 | 多光标移动、快捷键、剪贴板分配及单步撤销 |
| converter | 原 HEX 状态机、ASCII 编码和信息表共享；Qt 数值面板 | NUL、进制转换、非法字符处理、32 位溢出语义 |
| EditorConfig | 原缩进、最终换行、保存选项共享；core 0.12.10 + PCRE2 10.46 | 配置继承、glob、首行规则、撤销、保存命令顺序 |
| JSON Viewer | 直接编译原 JsonHandler、RapidJsonHandler 和固定 RapidJSON 分支 | 数值精度、注释/逗号、排序撤销、树计数、路径、字节定位 |
| Goto Line Col | 原列号计算和 UTF-8 字节分析提为共享头；原 Unicode 数据表、配置与命令行选项语义 | 字节内部定位、Tab/字符列、边界、原字符名称、配置、闪烁恢复、命令行一次性定位 |

## 尚未完成（不可计为已移植）

codealignment、DoxyIt、JsonToolsNppPlugin、
npp-session-manager、plugindemo、RandomValuesNPP、xmltools。

## 原版边界修复

- Reverse Lines：空长度和局部 CRLF 不再造成无符号下溢；逐字节复制保留 NUL。
- SelectQuotedText：限制样式扫描文档边界；Qt 入口使用指针宽度位置。
- MIME：平台头依赖改为标准类型；Base64 传实际文本字节数，不包含 Scintilla 传输 NUL；
  保留原算法的非严格解码语义。SAML 修正负值被无符号吞掉和异常分支泄漏，
  原 tinflate 增加显式输入/输出边界检查，不替换解压算法。
- Indent：直方图索引边界修复，不改变 5000 行限制、4 倍阈值和除数权重。
- 宿主：保留 PluginManager 管理职责，增加可选 nppGetCommandState 导出查询；
  QAction 只呈现状态，业务开关属于各插件。命令数组结构大小未变。
  通知尾部追加可选 lines_added，新增缩放事件；按 struct_size 兼容旧通知。
- SecurePad：修正原密文长度与内存分配边界，密文格式和原 Blowfish 算法不变。
- ElasticTabstops：保留原 int 算法位置类型，Qt 入口拒绝超出 int 范围的文档；
  修复禁用后换视图再启用时的旧编辑器引用，配置关闭后清除制表位。

## 验证环境与限制

Windows x64、Qt 5.12.12、MinGW 7.3。其他平台尚未实际构建，不能声称已验证。
ABI 测试使用可控宿主消息适配器和真实动态库、Qt 剪贴板。
另外链接宿主 npp-scintilla-qt 的真实编辑器测试，覆盖上述 17 个插件的关键行为。
Windows 测试使用实际 windows 平台后端：本机 Qt 5.12 offscreen/minimal 在 QMessageBox
显示时卡住、无法进入定时器回调；换用 windows 后端后括号弹窗测试通过。
动态库加载使用与宿主一致的 PreventUnloadHint；逻辑 SHUTDOWN/重新初始化单独验证。

2026-09-08：插件构建通过；36/36 项 CTest 通过（17 项 ABI、17 项真实 Scintilla、1 项命令行、1 项原算法回归）。
宿主完整回归此前出现部分 Qt 测试进程超时，尚未证实原因，不能计为全套通过。

依赖准备方式见 `README.md`。JSON Viewer 保留原 undefined 替换正则及排序回退语义，
修复 SAX 解析失败时未释放的栈节点，并为 undefined 树重试传入有效 TrackingStream。

EditorConfig 文件打开边界使用 UTF-8 → 宽字符路径适配，中文目录下的配置继承已通过实测。

Goto Line Col：保留原 Unicode 数据的字符名称及 U+ 十六进制格式；未替换为 Qt 的字符命名。
原线程等待改为 Qt 单次定时器，仍不重复启动正在执行的光标闪烁；关闭插件时恢复原编辑器。
命令行由 Qt 提供已分词的启动参数，保留首个 -n/-c、文件匹配、当前行约束和非持久化消费规则。
原 snprintf 追加到同一缓冲区产生的源/目标重叠改为临时缓冲区，不改变显示内容。
