# 阶段三：高完成度 UI 与视图骨架

执行日期：2026-07-22

## 本次范围

阶段三完成原版主要 UI 入口的骨架补齐。核心原则是：可执行功能保持现有行为，未实现功能以 disabled 占位呈现，避免用户误以为功能已经完成。

## 已完成 UI 面

| 区域 | 当前状态 | 说明 |
| --- | --- | --- |
| File 菜单 | 骨架已扩展 | 原版主要入口已出现，复杂命令禁用 |
| Edit 菜单 | 骨架已扩展 | Insert、Blank Operations、Auto-Completion 等分组已出现 |
| Search 菜单 | 骨架已扩展 | Find/Replace/Bookmark 可用，Find in Files/Mark 等占位 |
| View 菜单 | 骨架已扩展 | 面板、工具栏、状态栏、显示符号、折叠分组已出现 |
| Encoding 菜单 | 已有基础实现 | Encode in / Convert to / Character sets 已有结构 |
| Language 菜单 | 已有基础实现 | 按字母分组，当前为静态常用语言列表 |
| Format 菜单 | 已有基础实现 | EOL Conversion 可见并共享动作 |
| Macro 菜单 | 已有入口 | 录制/播放/保存/加载入口存在，实际完整性后续验证 |
| Plugins 菜单 | 已有边界 | 插件未加载时显示禁用占位 |
| Settings 菜单 | 骨架已扩展 | Preferences 可用，其余配置入口占位 |
| Run 菜单 | 新增骨架 | Run... 占位 |
| Window 菜单 | 新增骨架 | Windows 和排序/复制入口占位 |
| Help 菜单 | 骨架已扩展 | About 可用，文档/调试入口占位 |
| 工具栏 | 已有基础实现 | 标准工具栏从配置初始化显示/隐藏 |
| 状态栏 | 已有基础实现 | 六区状态栏从配置初始化显示/隐藏 |
| 停靠面板 | 已有基础入口 | 文件浏览器、文档地图、函数列表、查找结果已有 dock |
| 双视图 | 已有基础入口 | Split、Rotate、Move、Clone 已有入口 |

## 当前明确占位

- 打印、Save Copy As、Rename、删除文件、会话文件导入/导出。
- Close Multiple Documents 的细分命令。
- Insert 日期时间、自定义插入。
- 高级注释、大小写、空白处理、自动完成命令。
- Find Next/Previous、Find in Files、所有打开文件查找、Mark 面板。
- Always on Top、Full Screen、Post-It、Document List、Project Panels、Clipboard History、Character Panel、Fold All。
- Style Configurator、Shortcut Mapper、Edit Popup ContextMenu、Import。
- Run、Window 高级命令。
- Help 中在线文档、命令行参数、Debug Info。

## 阶段 4 输入

阶段 4 应单独进行原版 UI 一致性检验，重点比较：

- 菜单顺序、分组、文案和 enabled/disabled 状态。
- 工具栏图标顺序、尺寸和显示/隐藏配置。
- 状态栏分区、显示文本和动态刷新。
- 标签栏、双视图、停靠面板的可见结构。
- 哪些入口 UI 已存在但逻辑缺失，哪些入口 UI 仍缺失。
