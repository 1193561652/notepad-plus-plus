# Notepad++ v8.4.6 插件清单调查基线

日期：2026-07-30
状态：GitHub v1.5.4 三架构清单已归档，逐插件调查待执行

## 1. 目的与范围

本文为插件兼容性调查建立可复现的 v8.4.6 清单基线。范围包括：

- 确认 v8.4.6 使用的插件清单来源、载体和字段；
- 确定 x86、x64、ARM64 清单的合并规则；
- 规定后续插件 API、移植难点和兼容等级调查所需字段；
- 记录 GitHub 历史清单的来源、哈希和与发行包证据之间的边界。

本文不评价具体插件。三份 JSON 已作为原始材料加入仓库，逐插件 API 和兼容难度
仍使用单独调查记录。

## 2. 基线

- Notepad++ 基线 tag：`v8.4.6`
- tag commit：`6750d4dbbcf3f6c675d03d627277ae75b1d7ed4f`
- tag 日期：2022-09-29
- 目标架构：Windows x86、x64、ARM64
- 清单集合：上述三种架构随 v8.4.6 发布的插件清单并集
- GitHub 清单 tag：`nppPluginList/v1.5.4`
- tag commit：`1903cffe98207c6a5cc4cc6d370dbc36032f27fa`
- tag 日期：2022-09-23

插件的身份合并键暂定为清单中的 `folder-name`。同一 `folder-name` 在不同架构
清单中仍作为一个插件记录，但必须分别保留每个架构的版本和原始元数据，不能先
假定三个架构使用相同版本或下载包。

## 3. 仓库内可核验的事实

### 3.1 清单不在 Notepad++ 主仓库中

对当前工作树、`v8.4.6` tag 和本地用户目录进行查找，未发现：

- `nppPluginList.json`
- `nppPluginList.dll`
- 其他包含实际插件条目的插件列表 JSON

`v8.4.6` 的主仓库因此不能单独还原插件名称、插件版本或条目数量。主仓库只包含
列表消费者、安装打包引用和外部项目地址。

### 3.2 Release 与 Debug 的清单载体

`PowerEditor/src/WinControls/PluginsAdmin/pluginsAdmin.cpp` 显示：

- Debug 构建从插件配置目录读取 `nppPluginList.json`；
- Release 构建从插件配置目录读取 `nppPluginList.dll`；
- Release DLL 的 JSON 位于资源 ID `101`、资源类型 `256`；
- Plugin Admin 界面指向外部项目
  `https://github.com/notepad-plus-plus/nppPluginList`。

Release 代码会先验证清单模块和 `gup.exe`，再从 DLL 资源解析 JSON。这说明随
v8.4.6 各架构发行包交付的 `nppPluginList.dll` 是还原“用户当时实际看到的清单”
的首选证据。

### 3.3 架构清单分别打包

`PowerEditor/installer/nsisInclude/binariesComponents.nsh` 分别从以下位置打包
清单：

- x86：`bin/plugins/Config/nppPluginList.dll`
- x64：`bin64/plugins/Config/nppPluginList.dll`
- ARM64：`binarm64/plugins/Config/nppPluginList.dll`

`PowerEditor/installer/packageAll.bat` 也分别签名并复制这三个文件。因此不能用
一个架构的列表代替另外两个架构；三者都应采集，最终取并集。

## 4. v8.4.6 解析器接受的数据结构

根对象包含：

| 字段 | 要求 | 含义 |
| --- | --- | --- |
| `version` | 必需，字符串 | 插件列表版本 |
| `npp-plugins` | 必需，数组 | 插件条目 |

每个插件条目由 `loadFromJson()` 读取以下字段：

| 字段 | 要求 | 含义 |
| --- | --- | --- |
| `folder-name` | 必需 | 插件目录名，也是原版用于关联已安装插件的主要标识 |
| `display-name` | 必需 | Plugin Admin 显示名称 |
| `author` | 必需 | 作者 |
| `description` | 必需 | 描述 |
| `id` | 必需 | 插件包 SHA-256 |
| `version` | 必需 | 清单所列插件版本 |
| `repository` | 必需 | 插件包下载地址 |
| `homepage` | 必需 | 插件主页 |
| `npp-compatible-versions` | 可选 | 当前插件版本兼容的 Notepad++ 版本区间 |
| `old-versions-compatibility` | 可选 | 旧插件版本区间与其兼容的 Notepad++ 版本区间 |

版本区间解析器接受精确版本和闭区间写法，例如 `6.9`、`[4.2,6.6.6]`、
`[8.3,]`、`[,8.2.1]`。采集时必须保留原始字符串，不应只保留解析后的上下界。

注意：`PluginUpdateInfo` 中还存在 `_sourceUrl` 成员，但 v8.4.6 的
`loadFromJson()` 没有从列表条目读取对应字段。后续源码地址需要从主页、插件
仓库或人工调查补充，不能把它当作 v8.4.6 清单的已知字段。

## 5. 已取得的 GitHub 历史清单

来源仓库：`https://github.com/notepad-plus-plus/nppPluginList.git`

选择正式 tag `v1.5.4`，而不是 2022-09-28 的未发布开发态 HEAD。该 tag 在
Notepad++ v8.4.6 发布前 6 天创建，是当前从 nppPluginList 仓库取得的正式历史
基线。

| 架构 | 仓库路径 | 条目数 | 原始 JSON SHA-256 |
| --- | --- | ---: | --- |
| x86 | `src/pl.x86.json` | 169 | `5b081d5586bc031364091f058d2c3063e6d85d5cab0628bdc678e60ffb36d31d` |
| x64 | `src/pl.x64.json` | 134 | `5bd0e55a2f5c6c286e70b51a614a83611035062a0cf2a70308da6bddb3ddaa4e` |
| ARM64 | `src/pl.arm64.json` | 20 | `b615a62952d0806220ccf4728dc8b2f8e55b456a8f703277a8df4114461888ac` |

仓库内原样归档位置：

- `third_party/nppPluginList/catalog/windows/pl.x86.json`
- `third_party/nppPluginList/catalog/windows/pl.x64.json`
- `third_party/nppPluginList/catalog/windows/pl.arm64.json`

三份根 `version` 均为 `1.5.4`，必需字段缺失数和单清单重复目录数均为 0。按
`folder-name` 忽略大小写合并后共有 178 个插件：

- 三种架构都有：20；
- x86 与 x64 都有（含上述 20）：125；
- 仅 x86：44；
- 仅 x64：9；
- 仅 ARM64：0。

有 3 个同名插件在不同架构清单中的版本不同：

- `NewFileBrowser`：`0.1.3` / `0.1.5`；
- `NppToolBucket`：`1.10.6622.41336` / `1.10.6622.41516`；
- `PoorMansTSqlFormatterNppPlugin`：`1.6.13.31502` / `1.6.13.31508`。

当前清单是 GitHub 官方 tag 原文，但尚未逐字节对比 v8.4.6 三个官方发行包内
`nppPluginList.dll` 的资源。若后续比较发现差异，应以发行包内资源为最终发行
事实，并保留本次 GitHub tag 数据作为来源记录。

## 6. 后续发行包复核方法

### 6.1 首选：从 v8.4.6 官方发行包提取

分别取得 v8.4.6 的 x86、x64、ARM64 官方发行包，并执行：

1. 记录发行包文件名、来源、下载日期和 SHA-256；
2. 从每个包中取得 `plugins/Config/nppPluginList.dll`；
3. 记录 DLL SHA-256；
4. 提取资源 ID `101`、类型 `256` 的原始字节；
5. 将末尾终止符与 JSON 有效载荷区分开，保存未经格式化的原始 JSON；
6. 解析后记录根 `version`、条目数和解析失败条目；
7. 校验每个条目的所有必需字段及 `id` 是否为 64 位十六进制 SHA-256；
8. 对三个架构按 `folder-name` 合并，同时保留各自原始条目。

提取工具不构成基线的一部分；可以使用能够读取 PE 资源的现成工具或小型只读
脚本。无论使用何种工具，都必须保留 DLL 与提取结果的哈希以便复核。

GitHub `v1.5.4` 数据已经满足当前开发和调查基线；上述发行包提取只用于确认
v8.4.6 用户实际收到的 DLL 资源是否与 tag 完全一致。

## 7. 插件清单记录格式

取得原始清单后，每个插件至少记录：

| 字段 | 说明 |
| --- | --- |
| `folder_name` | 原始 `folder-name`，插件合并键 |
| `display_name` | 原始显示名 |
| `listed_architectures` | x86、x64、ARM64 中出现的位置 |
| `listed_versions` | 按架构保存的插件版本 |
| `list_versions` | 条目来源的列表版本 |
| `repositories` | 按架构保存的包地址 |
| `package_sha256` | 按架构保存的原始 `id` |
| `npp_compatibility` | 原始兼容区间字符串 |
| `old_version_compatibility` | 原始旧版本兼容字符串 |
| `homepage` | 清单主页 |
| `author` | 清单作者 |
| `description` | 清单描述 |
| `provenance` | 来源 DLL、JSON 和哈希 |

原始字段与调查结论应分开保存。清单元数据不得被后续主观评价覆盖。

## 8. 插件调查记录格式

在清单整理完成后，为每个插件增加以下调查字段：

| 分组 | 字段 |
| --- | --- |
| 重要性 | 使用广泛程度、判定证据、是否列为重点插件 |
| 源码 | 有无源码、源码地址、许可证、对应 tag/commit、构建可复现性 |
| 二进制 | 架构、导出函数完整性、运行时和第三方依赖 |
| NPP API | 使用的 `NPPM_*`、`NPPN_*`、命令 ID 和参数语义 |
| Scintilla API | 使用的 `SCI_*`、`SCN_*`、指针/缓冲区/Direct Function 依赖 |
| Win32 | HWND 获取、窗口遍历、控件类、subclass、Hook、消息及原生 UI |
| 高级能力 | Dock、工具栏、对话框、脚本运行时、子进程、网络、文件系统 |
| 代理模型 | 主窗口与两个 Scintilla 代理窗口可覆盖的调用、缺失语义 |
| 难点 | 编码、结构体布局、线程、重入、生命周期、性能和 UI |
| 结论 | A/B/C/D/U 兼容等级、证据、所需宿主能力、未验证项 |

无源码插件必须显式标为“无源码”，不进行反汇编。重要的无源码插件另行标注，
后续可通过原版应用的外部可观察行为和交互进行功能复刻；在取得证据前兼容等级
应保持 `U`，不能因能下载到 DLL 就推断其 API 使用情况。

## 9. 完成条件

本轮“插件清单整理”只有同时满足以下条件才算完成：

- 已取得并归档 v8.4.6 x86、x64、ARM64 三份清单；
- 每份清单均有来源与 SHA-256；
- JSON 可由严格解析器完整读取，失败项有单独记录；
- 已输出按 `folder-name` 合并的并集；
- 每个合并记录保留逐架构原始元数据；
- 条目数量、重复键、缺失字段和架构差异均有统计；
- 没有混入无法证明属于 v8.4.6 的插件条目。

当前已完成三份 GitHub 历史 JSON 的归档、哈希、严格解析和基础架构差异统计。
按 `PS-030`，逐插件源码、API 和兼容难度调查只处理 x86 清单中的 169 项；x64
和 ARM64 不进入本轮调查。169 项首轮静态调查现已完成，索引、分批记录和统计见
[`plugins-v846/README.md`](plugins-v846/README.md)；重要度首评见
[`plugins-v846/importance.md`](plugins-v846/importance.md)。发行包 DLL 资源
复核和 Windows x86 实机验证仍是待办项。
