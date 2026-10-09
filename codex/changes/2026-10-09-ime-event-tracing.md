# 输入法删除与焦点切换事件诊断

用户报告 Ubuntu 系统拼音输入未确认后逐个退格清空，切换前后台后首字母重现，
文档变为未保存。本次仅添加可选诊断，不改变输入、撤销或焦点行为。

## 原版结构与修改边界

- 已核验 v8.4.6 的 `scintilla/qt/ScintillaEditBase/ScintillaEditBase.cpp`：
  `inputMethodEvent` 负责通过 Document tentative undo 管理预编辑文本。
- Windows 原版 `ScintillaWin::FocusMessage` 在失焦时协调 IME 完成，
  `ImeEndComposition` 清理 tentative 状态；Qt 当前没有同样的原生事件生命周期。
- 日志仍位于 Qt 平台包装器 `ScintillaEditBase`，不转移状态所有权，
  不在 MainWindow 中实现输入法行为。

## 日志使用

启动前设置 `NPP_IME_TRACE` 为可写的文件路径。未设置时不采集。
每行一个 JSON 对象，立即 flush，文件以追加方式打开。

记录按键、输入法、焦点事件处理前后状态，应用活跃状态变化，以及
Scintilla 文本修改／保存点通知。字段包括全局序号、时间、进程／控件／文档标识、
preedit、commit、替换范围、输入法属性、退格按键、焦点原因、光标、
tentativeActive、modified、文档长度及前 256 字节（UTF-8 显示与十六进制）。
日志包含输入文本，复现使用空白新文档；超过 256 字节的文档有截断标记。

典型启动参数：`-multiInst -nosession -titleAdd="输入法日志复现"`。
不强制改变 QT_IM_MODULE，以保持原输入法环境。

## 判读

逐个退格后的最后一份非空预编辑文本通常为首字母。检查清空事件后是否存在
再次送入的 preedit 或 commit；同时区分没有新事件但文档变化、仅重绘显露残留。
Qt 5 IBus hide/show 缓存是待验证假设，不能仅凭模拟事件认定真实根因。

## 本次验证

- `build-ubuntu-package` 的 `notepadpp-qt` 目标构建成功。
- 用独立 Qt 事件探针驱动新编译的编辑器库，103 条 JSON 记录均可解析；
  验证事件顺序、空预编辑撤销、焦点事件、缓存重送、正式提交及修改通知。
- 桌面程序已以 xcb / ibus 启动，日志确认窗口已激活并获得焦点。
  用户已完成真实输入法复现，结果如下。

## 真实复现结果

日志：`/tmp/npp-ime-repro-20261009-fydw4hkv/events.jsonl`，进程 14414，
Qt 5.15.3 / xcb / ibus；本次采集 168 条记录。

| 序号 | 时间 | 事件与状态 |
| --- | --- | --- |
| 131 | 09:31:24.125 | 最后一次非空 preedit 为“年”，commit 为空。 |
| 132–136 | 09:31:24.514–515 | 收到空 preedit / commit，处理后文档确实为空，modified=false，tentativeActive=false。 |
| 139–144 | 09:31:27.037–038 | 收到 preedit 为空、commit="年" 的正式提交事件；文档从空变为“年”，modified=true，tentativeActive=false。 |
| 145–152 | 09:31:27.038 | 应用变为 Inactive，然后收到 WindowDeactivate 和 FocusOut。 |
| 153–160 | 09:31:27.925–927 | 返回前台，文档保持“年”，没有新的输入法事件。 |

因此已确认：清空正常完成，字符由切后台过程中的正式提交重新写入，
并非焦点返回时重绘残留。本次重现的是最后一个预编辑候选字“年”，不是拉丁字母 n。

Qt 5.15 源码解释了与日志一致的触发链：

1. IBus `hidePreeditText()` 发送空事件，但保留 `d->predit`。
2. `QApplication::setActiveWindow()` 在发送失焦通知前调用
   `QGuiApplication::inputMethod()->commit()`。
3. IBus `commit()` 把缓存 `d->predit` 作为 commitString 发回编辑器。

来源：
- https://github.com/qt/qtbase/blob/5.15/src/plugins/platforminputcontexts/ibus/qibusplatforminputcontext.cpp
- https://github.com/qt/qtbase/blob/5.15/src/widgets/kernel/qapplication.cpp

日志直接证明编辑器收到正式提交；IBus 内部 hide/cache 调用链依据 Qt 源码推断，
尚未对平台插件做函数级追踪。修复不能仅放在 focusOutEvent 中，因为错误提交
发生在它之前。后续应研究取消组合时同步输入法状态，并验证合法提交不会被误丢弃。

## 修复与回归验证

在 `ScintillaEditBase::inputMethodEvent` 中，已有 tentative 输入收到空 preedit、
空 commit 且无替换操作时，先完成原有 `TentativeUndo`，再对仍由本控件持有的
Qt 输入上下文调用 `QInputMethod::reset()`。这样同步取消 Qt 缓存和输入法状态，
避免窗口切换自动提交旧缓存。先清除 tentative 状态也避免 reset 回调再次触发 reset。
不在焦点处理或 MainWindow 中拦截提交，不按内容黑名单过滤字符。
预编辑结束时把 `preeditPos` 还原为 -1，候选框查询重新使用当前光标。

原版 Document / Editor 的临时撤销、保存点和提交职责不变；Qt 平台层增加
取消组合时的上下文同步。诊断日志保留且默认关闭，新增 reset 前后记录。

新增 `ime-composition-tests`：真实采集的候选序列取消、保存点恢复、焦点往返、
候选框位置、保留已修改文档及撤销历史、正常确认后 hide、取消后的独立提交、
再次组合输入和非 BMP 字符。主程序和测试构建通过，针对性 CTest 通过。
启用日志运行测试另确认三次取消触发 reset，调用前 tentative 状态均已结束。
offscreen 自动化不包含真实 IBus 缓存。修复版桌面日志
`/tmp/npp-ime-fixed-20261009-kr5tisbl/events.jsonl` 已确认：09:36:46.790
取消组合并 reset 后，09:36:49.351 切后台、09:36:54.949 回到前台，文档始终为空，
modified=false，没有再次收到旧字符提交。真实 IBus 下的删空／焦点往返复现通过；
正常确认等其他路径由自动化事件测试覆盖。

Ubuntu 包修订号从 qt2 升为 qt3，生成 `8.4.6-qt3` 安装包。
完整 Release 构建成功，38/38 CTest 通过；CPack DEB 生成成功，已检查包元数据、
文件清单，并解包确认 `/usr/bin/notepad++` 与本次构建二进制逐字节一致。
