# 插件无决策缺口索引

## 配置与状态

| 插件 | 所有者 | 当前行为 |
| --- | --- | --- |
| Explorer-qt | `FavoritesDocument` | 无损读取和局部修改 `Favorites.dat` 递归树 |
| ComparePlus-qt | 比较会话状态 | 跟踪主/副 Buffer，关闭时清理标记与定时器 |
| HEX-Editor-qt | 十六进制文档模型 | 源快照、脏状态、冲突检查和放弃确认 |

## 编辑与命令

| 插件 | 新增行为 | 保留边界 |
| --- | --- | --- |
| DSpellCheck-qt | 原版完整 Lexer/style 分类、脏行增量重检、URL/邮箱排除、右键建议和调试日志 | 无已知基础行为缺口；Aspell/在线词典属于独立依赖与产品范围 |
| NppExec-qt | LABEL/GOTO、嵌套块 IF、NPE_QUEUE、PROC_INPUT/SIGNAL | 脚本仓库、指针消息 |
| NppFTP-qt | 删除、重命名、建目录、重试、取消和队列状态 | 安全凭据与主密码迁移 |
| NPPTextFX2-qt | 引号、对齐、重排、缩进和小端字节转换 | Viz、键盘钩子、Tidy |

详细变更见 `codex/changes/2026-08-13-non-decision-plugin-gap-closure.md`。
