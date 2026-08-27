# ComparePlus 与 HEX-Editor 新 ABI 移植

## 宿主

- ABI v1 以 `struct_size` 兼容方式追加原始文档、选择区、Buffer/View、新建文档、
  剪贴板和状态栏回调。
- 为深层编辑器交互继续在结构尾部追加按视图读取文档、在主副视图显示 Buffer、差异
  标记、首个可见行和跳转回调；接口保持 C 类型和平台无关语义。
- 二进制写入使用显式长度 Scintilla 消息，保留嵌入 NUL。
- 标准插件路径先探测 `nppGetPluginAbiVersion`；新旧 ABI 同时存在时使用新 ABI，只有
  旧 ABI 时回退 Windows 原版加载器。

## 插件

- `ComparePlus-qt` 从 `cp_1.0.0` 建立，复用原版 `DiffCalc`，保留命令表、比较状态、
  差异导航、过滤、统计、剪贴板及 Git/SVN/最后保存版本入口。
- ComparePlus 深层交互包括：把两份 Buffer 放入永久主副编辑器、在编辑器背景标记
  Added/Removed/Changed/Moved 行、同步垂直滚动、差异跳转和可点击导航缩略条。
- `HEX-Editor-qt` 从 `0.9.12` 建立，提供二进制安全的虚拟表格编辑、查找、定位、
  模式替换、比较、列插入和选项。
- HEX-Editor 深层数据操作包括：1/2/4/8 字节位宽、大小端转换、二进制显示与回写、
  行书签导航，以及独立于普通文本编辑的块复制、剪切、粘贴和删除。
- 两个插件名均带 `-qt`，并提供原版六导出空实现供原版宿主安全检查。

## 验证

- 两个插件仓库的核心算法/模型测试与 ABI 动态加载烟雾测试通过。
- 烟雾测试实际解析并调用新 ABI，同时调用旧 `getName/getFuncsArray`，确认旧接口返回
  `-qt` 名称和零命令表。
- 宿主完整构建通过，完整 CTest `47/47` 通过；包括跨平台 ABI、插件管理、Win32
  插件联合加载、配置、会话、编辑器和 UI 自动化回归。

## 后续交互验证

2026-08-27 已完成 Ubuntu Qt 控件级最终回归：ComparePlus 覆盖差异表、连续导航、
导航缩略条、双视图标记、滚动同步、设置、自动重比较和真实临时 Git 仓库；HEX-Editor
覆盖含 NUL 回写、表格编辑、Apply、查找/定位、模式替换、比较着色、位宽、端序、
二进制模式和书签。详见
`codex/changes/2026-08-27-compareplus-hexeditor-interaction-regression.md`。

SVN 客户端在本次 Ubuntu 环境中不可用；Windows/macOS 的平台外观和原生 Git/SVN
安装差异仍应在对应发布平台检查，不属于 Ubuntu 回归失败。
