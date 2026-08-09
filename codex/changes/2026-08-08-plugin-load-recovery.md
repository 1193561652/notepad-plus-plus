# 2026-08-08 插件加载恢复与回滚

## 改动

- 新增跨平台 `PluginLoadJournal`，使用 JSON Lines 记录加载会话和结果，并用原子标记
  保存当前正在加载的插件。
- Windows 加载器在异常启动后跳过嫌疑插件一次，同时继续加载其余白名单插件并显示提示。
- DLL 校验或注册失败时释放模块、恢复命令 ID 检查点，不留下半注册插件。
- `-noPlugin` 启动不再创建 Win32 插件管理器、代理 HWND 或加载状态文件。
- PE 架构和依赖预检按本轮决策延期。

## 数据结构

- `<userPath>/plugin-load/plugin-load.jsonl`：会话、加载开始、成功、失败、恢复跳过、完成事件。
- `<userPath>/plugin-load/plugin-load-in-progress.json`：崩溃恢复标记，使用 `QSaveFile` 原子提交。
- 日志大小超过 1 MiB 后保留一份 `.1` 轮转文件。

## 验证

- `plugin-load-journal-tests`：模拟加载中进程退出，验证下次会话恢复、跳过和标记清理。
- `ui-plugin-load-recovery`：预置 mimeTools 中断标记后启动完整宿主，验证仅跳过嫌疑插件、
  其余插件继续加载、恢复提示出现且标记清除。
- `ui-plugin-registration-rollback`：无效 DLL 注册失败后加载有效 DLL，验证插件集合、标记和命令 ID 回滚。
- `ui-no-plugin-runtime`：通过真实命令行解析器解析 `-noPlugin`，验证宿主不初始化插件系统。
