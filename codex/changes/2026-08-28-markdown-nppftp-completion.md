# Markdown 扩展语法与 NppFTP 状态层收口

## Markdown 插件

- 两个插件统一以 Qt CommonMark/GFM 解析器处理基础块和内联语法。
- 对齐原版 Markdig `UseAdvancedExtensions()`：缩写、自动标题 ID、引用、容器、定义
  列表、扩展强调、图、页脚、脚注、网格/管道表、数学、媒体、任务、图表围栏、
  自动链接和通用属性。
- 扩展生成的 HTML 使用占位符跨过基础解析器，避免被二次转义。
- 按原版 Markdig 输出收口运行时：Mermaid/nomnoml 围栏生成静态 `<div>`，数学公式
  保留 `\(...\)`/`\[...\]`；原版不内置也不联网加载 Mermaid、nomnoml、MathJax
  或 KaTeX，因此 Qt 版同样不注入额外 JavaScript。
- MarkdownViewerPlusPlus 保留内嵌基础 CSS 与用户 `@import` 的原版顺序；
  NppMarkdownPanel 随插件安装原版 `style.css`，并在外部文件缺失时使用内嵌副本。

## NppFTP

- 新增 `CredentialVault`：PBKDF2-HMAC-SHA256 200,000 次派生密钥，AES-256-GCM
  认证加密，每个密文使用独立随机盐和 nonce；错误密码及密文篡改都会失败。
- 主密码只保留在进程内。取消解锁时禁止写回 profile，避免空凭据覆盖已有密文；
  原 Base64 profile 可读取并在启用主密码后迁移。
- 传输队列显示 queued/running/retry/completed/failed/cancelled 状态，失败最多重试三次，
  支持取消或重试选中项以及取消全部。
- `CacheIndex` 将 profile、远端路径、本地路径和最近上传哈希持久保存为 JSON；缓存路径
  使用 SHA-256 隔离，启动时清理失效索引，并支持用户确认后清空缓存。

## 验证

- Ubuntu Qt 5 / OpenSSL 3 构建。
- MarkdownViewerPlusPlus `2/2`、NppMarkdownPanel `1/1`、NppFTP `2/2` CTest；
  Markdown 测试明确覆盖原版静态图表与数学标记。
