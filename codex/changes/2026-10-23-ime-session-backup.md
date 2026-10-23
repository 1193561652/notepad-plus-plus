# 输入法取消后关闭／重启恢复旧候选字

## 复现与原因

已安装版本为 8.4.6-qt4。此前修复的是取消输入后 Qt/IBus 在失焦时重新提交缓存，
此次用户确认切换前后台正常，但关闭程序再打开仍会恢复已删除的候选字。

新增真实 MainWindow 多进程回归：发送“年”的预编辑事件，主动触发快照定时器，
发送空输入法事件取消组合，然后正常关闭窗口。修改前测试以退出码 9 失败，
确认清空且恢复保存点的文档仍保留候选字快照文件及备份路径。

`onBackupTimer` 原来直接跳过所有非 dirty buffer；`saveSession` 却继续写入旧
备份路径。启动时 `restoreSession` 读取该路径，导致旧候选字重新成为未保存正文。
这是独立于 Qt/IBus 提交缓存的会话备份同步缺陷。

## 原版职责与修改

已核验 v8.4.6 `PowerEditor/src/ScintillaComponent/Buffer.cpp` 中
`FileManager::backupCurrentBuffer`：文档恢复为非 dirty 时删除旧备份、清空路径，
随后保存会话。Qt 既有快照遍历入口为 `MainWindow::onBackupTimer`，
磁盘备份清理由 `Buffer::clearBackupFile` 负责，会话仍由原有 save/restore 路径管理。
本次仅在既有遍历中补调用 `clearBackupFile`，不搬迁任何子系统或增加配置格式。
关闭时原有顺序为备份同步、会话保存，因此清理后的路径不会再进入 session.xml。

仍 dirty 的文档继续写入当前内容，保留合法未保存正文。大文件限制保持原状。
不改变已有输入法事件处理，不丢弃正式提交，不清空用户当前文档。

## 回归与打包

`ime-session-restart` 使用临时配置和四个独立进程，覆盖空文档取消输入后关闭重启，
以及已有未保存正文 keep、组合候选字、备份、取消输入、关闭重启后保留 keep。
未修改时测试失败，修复后完整 Release 构建成功，40/40 CTest 全部通过。
Ubuntu 包版本升为 8.4.6-qt5，已生成 amd64 DEB，检查文件清单及包元数据，
解包确认主程序与本次构建逐字节一致。
