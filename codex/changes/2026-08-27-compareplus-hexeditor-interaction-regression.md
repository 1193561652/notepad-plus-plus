# ComparePlus 与 HEX-Editor 最终交互回归

日期：2026-08-27

## Ubuntu 验证结果

- ComparePlus：动态加载插件后，通过真实 Qt 控件验证差异表、颜色、前后连续导航、
  导航缩略条、仅显示差异、主副视图标记、垂直滚动同步、设置持久化和自动重比较。
- ComparePlus：在测试期间创建并提交真实临时 Git 仓库，验证 `Git Diff` 读取 `HEAD`
  并生成比较结果。
- HEX-Editor：通过真实虚拟表格和按钮验证含 NUL 的编辑/Apply 往返、查找选择、模式
  替换、比较着色、4 字节位宽、小端和二进制显示、偏移定位及书签。
- 两个插件均使用 Linux/Ubuntu 宿主信息动态加载；Release 构建及各自 2/2 CTest 通过。

## 边界

本次环境没有 SVN 客户端，因此没有执行真实 SVN BASE 比较。Windows/macOS 原生窗口
外观和各平台 Git/SVN 安装差异仍需在对应发布环境复核；核心跨平台交互已在 Ubuntu
自动化执行，而不是只验证模型函数。

插件实现说明分别见同级仓库 `../comparePlus/qt/README.md` 和
`../NPP_HexEditor/qt/README.md`。
