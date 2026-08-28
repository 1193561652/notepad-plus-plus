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
| DSpellCheck-qt | 脏行增量重检、URL/邮箱排除、合并多语言建议、本地词典和用户词典管理 | Aspell/在线下载属于独立依赖与产品范围 |
| NppExec-qt | LABEL/GOTO、嵌套块 IF、NPE_QUEUE、PROC_INPUT/SIGNAL、`npes_saved.txt` 脚本仓库、NPP_EXEC、SCI 指针消息 | 无已知的无决策基础行为缺口；任意 NPP 原生窗口消息不进入跨平台 ABI |
| NppFTP-qt | 认证加密主密码、持久缓存、可见队列、逐项重试/取消、远程编辑操作 | 系统钥匙串和跨设备凭据同步不属于原版模型 |
| NPPTextFX2-qt | 84 项可移植命令，覆盖引号、对齐、重排、缩进、编码、括号和插入工具 | 32 位 Viz、Win32 钩子、Tidy、原始单字节转码 |

详细变更见 `codex/changes/2026-08-13-non-decision-plugin-gap-closure.md`、
`codex/changes/2026-08-28-dspellcheck-textfx-completion.md` 和
`codex/changes/2026-08-28-markdown-nppftp-completion.md`。
