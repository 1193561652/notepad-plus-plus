# 2026-08-13 无需技术决策的插件缺口补齐

本批次只处理沿现有架构和 ABI 能够直接实现的行为，不引入新的运行时、解析器、
凭据后端或宿主 UI ABI。

## 已完成

- Explorer：新增 `FavoritesDocument`，无损保留 UTF-16LE `Favorites.dat` 的四个根、
  递归 `#GROUP/#END`、展开字段和未知行；增删收藏只修改目标 `#LINK`。
- DSpellCheck：按完整脏行增量重检，平移未受影响标记，排除 URL/邮箱，恢复调试日志。
- ComparePlus：跟踪比较双方 Buffer，关闭文档时清理标记和定时器，区分 Clear Active/All。
- HexEditor：增加源快照、脏状态、放弃确认和外部修改冲突保护。
- NppExec：增加 LABEL/GOTO、IF-GOTO、嵌套 IF/ELSE IF/ELSE/ENDIF、NPE_QUEUE、
  PROC_INPUT 和 PROC_SIGNAL。
- NppFTP：增加 FTP/SFTP 删除、重命名、建目录、连接重试、队列状态和取消。
- NPPTextFX2：增加 12 项原版平台无关文本与字节转换命令。

## 明确保留

- PythonScript 嵌入式 Python 版本选择、Markdown 标准解析器、NppFTP 安全凭据后端。
- 跨平台 ABI 的动态上下文菜单、工具栏和动态命令状态接口。
- DSpellCheck 全 Lexer/style 分类和上下文建议；NppExec 脚本仓库与指针参数消息；
  TextFX 的 Win32/x86 Viz、键盘钩子和 Tidy。

这些项目分别需要技术决策、新宿主能力或第三方依赖，不能在兼容层中模拟成“已完成”。
