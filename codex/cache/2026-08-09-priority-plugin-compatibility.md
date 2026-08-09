# 本地缓存：高、中优先级插件兼容

- 已接入且保留原始 DLL：GotoLineCol 2.4.2.0、RandomValuesNppPlugin 0.2.1、Merge files in one 1.2.0.0、SelectToClipboard 1.0.3、urlPlugin 1.2.0.0。
- 新增核心能力：主/副编辑器 `SCN_*` 同步桥接；Buffer/路径/编码/文档索引/状态栏/版本/主题/命令行等有限 `NPPM_*`；候选插件实际使用的有限 `SCI_*`。
- XMLTools 3.1.1.13 虽可安装和注册菜单，但 `NPPN_READY` 的线程键盘 Hook 在真实矩阵中导致卡死，按规则跳过。
- DoxyIt、SurroundSelection、ElasticTabstops 因 direct-call/Hook 跳过；SessionMgr、Linter、WakaTime 因需要较大的 Host Services、进程或隐私架构跳过。
- 详细证据：`codex/changes/2026-08-09-priority-plugin-compatibility.md`。
