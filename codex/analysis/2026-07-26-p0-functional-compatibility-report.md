# P0 功能兼容分析报告

日期：2026-07-26

| 功能 | 原版 v8.4.6 行为目标 | Qt 主线当前状态 | 非交互验证 | 仍需运行时确认 |
|---|---|---|---|---|
| URL Hotspot | 识别 URI、按配置高亮并可打开 | 已实现默认/自定义 scheme、indicator 和点击打开 | indicator 测试通过 | 系统默认应用、悬停视觉 |
| XML/HTML 标签匹配 | 标记配对标签及可选属性 | 已实现嵌套匹配、HTML 大小写规则和配置开关 | indicator 测试通过 | 复杂文档视觉和属性高亮细节 |
| 字符自动插入 | 按偏好插入括号、引号、闭合标签 | 已实现六类字符和 HTML/XML 闭合标签 | 圆括号测试通过 | 输入法、撤销、边界输入 |
| 保存前备份 | simple/verbose、自定义目录、时间戳 | 已实现并接入覆盖保存流程 | 路径和内容测试通过 | 失败提示、目录选择器 |
| InternalCommands | 用原版 ID 保存并应用菜单快捷键 | 已映射当前已实现 QAction | 配置往返测试通过 | Shortcut Mapper 和重启恢复 |
| ScintillaKeys | 支持主键和 NextKey | 已解析并应用到每个编辑视图 | 配置往返测试通过 | 冲突和多段键交互 |
| 宏命令序列 | 顺序执行 Scintilla 和菜单命令 | 已覆盖 type 0/1/2 | 构建和配置语料通过 | 混合序列及专用状态 |
| workspace XML | 原版 Project/Folder/File 结构 | 已完整读写和解析相对路径 | 专项测试通过 | 真实 workspace 交互 |
| Project Panels | 三个独立项目面板 | 已实现加载、保存和成员编辑 | 构建通过 | UI 操作、恢复和本地化 |
| Find in Projects | 仅搜索选中项目成员 | 已按面板掩码和成员集合搜索 | 构建通过 | 结果跳转和复杂筛选 |
| Replace in Projects | 仅替换选中项目成员并保护文件 | 已保留编码/BOM、原子写回并跳过风险文件 | 构建通过 | 确认框及真实文件流程 |

## 结论

P0 代码实现和自动化验证已完成。当前不能把 GUI 交互和像素级一致性标记为通过，因为按用户要求尚未启动程序；这些检查将在获得明确授权后执行。

## 运行时验证更新

用户授权后已执行真实 Qt Widget 捕获。主窗口、Project Panels、Find in
Projects、备份、标签匹配、自动插入和 Shortcut Mapper 均完成截图检查。

验证过程中发现 Shortcut Mapper 将所有 QAction 平铺并混入编码/语言动作。
该问题已修复为原版语义的四分类页，并补齐宏、Run commands 和
ScintillaKeys 快捷键写回。修复后 Debug 构建成功，CTest `18/18` 通过。

系统 URL 打开、输入法、目录选择器和真实 workspace 文件操作仍需要用户在
已启动的主程序中直接确认。
