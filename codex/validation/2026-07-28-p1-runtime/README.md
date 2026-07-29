# 2026-07-28 P1 运行验证报告

## 结论

- `ui-parity-capture.exe` 退出码 0。
- `p1-runtime-capture.exe` 退出码 0。
- Windows 文件关联管理员运行测试退出码 0。
- CTest 21/21 通过。
- 主程序已启动供用户人工检查。

## 自动 UI 与功能验证

| 功能 | 结果 | 证据 |
| --- | --- | --- |
| 19 个偏好设置页面 | 通过 | `baseline/preferences-page-*.png` |
| 文件关联页面布局 | 通过 | `baseline/preferences-page-07-File-Association.png` |
| Shortcut Mapper 四个标签页 | 通过 | `baseline/shortcut-mapper*.png` |
| Finder、标记复制、搜索宏 | 通过 | `functional/metadata.txt` |
| 高级排序与 Column Editor | 通过 | `functional/metadata.txt` |
| UDL 设计器与隔离写回 | 通过 | `functional/metadata.txt` |
| PDF 打印 | 通过 | `functional/print-output.pdf` |
| 外部修改、重命名、权限、删除 | 通过 | `functional/metadata.txt` |

## 文件关联真实验证

管理员测试使用随机扩展名 `.nppqb2b04578`：

- 注册为 `Notepad++_file`：通过。
- 在已注册扩展枚举中出现：通过。
- 打开命令指向本次 Qt 主程序并包含 `"%1"`：通过。
- 移除后恢复测试前 ProgID：通过。
- 退出后扫描 `.nppq*` 测试键：0 个残留。
- 测试前 `Notepad++_file` 的描述、图标和打开命令均由清理器恢复。

原始输出见 `file-association.txt`。

## 仍需环境或人工确认

- 真实 UNC/SMB 端点的断线与恢复。
- 物理打印机纸张输出；本轮 PDF 输出已通过。
- 用户对多 DPI、本地化、深色模式和原版像素一致性的最终观察。
- 用户在已启动主程序中对文件关联页及 Shortcut Mapper 的人工检查。
