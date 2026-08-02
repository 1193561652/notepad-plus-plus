# mimeTools compatibility fixture

This directory caches the real `mimeTools` 2.8 x64 plugin selected from the
official Notepad++ plugin list. It is used only by the Windows plugin ABI
compatibility test.

- Catalog: `resources/pluginList/windows/pl.x64.json`
- Release: `https://github.com/npp-plugins/mimetools/releases/tag/v2.8`
- Archive: `mimetools.v2.8.x64.zip`
- Archive SHA-256: `ed5133f8a0552e974135ada78a0260581be979f916ddbcd45697d2a1a1b8f280`
- DLL SHA-256: `b9a8ca258aa3edca1aa1b3ea4e264d3b0cda7c82a30b7464586d8be95701ea61`
- Architecture: PE x86-64
- Imports: `KERNEL32.dll`, `USER32.dll`

CTest verifies the archive hash, then passes a standard update plan to
`npp-plugin-updater`. The updater performs the same copy, package validation,
extraction, receipt, and transactional installation path used by Plugins
Admin. The application does not copy or load a DLL directly from this cache.

Do not replace the archive without updating the catalog selection, hashes,
ABI inspection, and runtime test together.
