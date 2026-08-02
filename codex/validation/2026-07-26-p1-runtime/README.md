# P1 运行验证报告

验证日期：2026-07-26

## 结论

- `ui-parity-capture.exe` 基线验证退出码 0。
- `p1-runtime-capture.exe` 最终退出码 0。
- Debug 全量构建成功。
- CTest 18/18 通过。

## 已验证

| 功能 | 结果 | 证据 |
| --- | --- | --- |
| Finder 结果、匹配高亮、导航数据 | 通过 | `functional/finder-results.png` |
| Copy Marked Text | 通过 | `functional/metadata.txt` |
| 保存搜索宏 type 3 | 通过 | `functional/metadata.txt` |
| 整数、小数逗号、矩形排序 | 通过 | `functional/metadata.txt` |
| Column Editor 进制/前导零/重复 | 通过 | `functional/column-editor.png` |
| UDL 设计器 | 通过 | `functional/udl-designer.png` |
| UDL 隔离文件保存/重载 | 通过 | `functional/metadata.txt` |
| PDF 打印、页眉页脚、变量、页码和行号 | 通过 | `functional/print-output.pdf`、`functional/print-output-page-1.png` |
| 外部修改提示 | 通过 | `functional/external-modified.png` |
| 外部重命名提示及路径更新 | 通过 | `functional/external-renamed.png` |
| 外部删除提示 | 通过 | `functional/external-deleted.png` |
| 文件只读/恢复可写状态 | 通过 | `functional/metadata.txt` |

## 验证中发现并修复

1. 全文行处理把以 EOL 结尾的 Scintilla 末尾占位行纳入排序，产生额外空行。
   `selectedLines()` 现排除该占位行。
2. Finder 结果同时绘制 QListWidgetItem 默认文本和富文本匹配标签，产生叠字。
   设置 item widget 后现清空默认显示文本。
3. 本机没有打印机队列，无法稳定捕获系统打印对话框。打印实现被提取为
   `ScintillaComponent/Printer` 生产组件，并由同一组件输出 PDF 完成视觉验证。

## 环境限制

- 本机没有物理或系统打印机队列，因此未执行真实纸张打印；PDF 输出已渲染检查。
- 本机没有可用 UNC/SMB 测试端点，因此未执行真实网络共享断线/恢复测试。
  定时轮询代码已构建，普通文件 watcher 与直接处理路径已验证。

## 产物

- `baseline/`：主窗口、查找页、偏好页和 Shortcut Mapper 基线截图。
- `functional/`：P1 功能截图、PDF、渲染页、元数据和执行进度日志。
