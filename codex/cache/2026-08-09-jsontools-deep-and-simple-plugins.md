# Cached state: JsonTools deep matrix and simple plugins

- JsonTools 3.2.0 deep matrix passes for Settings, RemesPath query/assignment,
  JSON Lines, JSON-to-YAML, tree source navigation, and the exact 4 MB partial-tree threshold.
- The 4,000,100-byte test runs only in `ui-localization-runtime-100` to keep the complete UI matrix fast.
- Upstream `Run tests` is not portable: `JsonGrepperTests.cs` calls `GetFiles()` on the author's absolute
  `C:\Users\mjols\...\testfiles\small` path without a guard. Do not intercept the command or fabricate
  that path in the host. A patched/upgraded plugin requires a separate version decision.
- Official nppConverter 4.4.0 and NppPluginDemo 4.2 x64 packages are installed through the existing
  managed corpus plan and loaded only from the audited Windows whitelist.
- The only new editor forwarding surface is `SCI_ADDTEXT` and `SCI_ENSUREVISIBLE`.
- Converter ASCII/HEX, config creation, native Conversion Panel insertion, Demo Hello, Demo Dock, and
  Demo line navigation pass against the real DLLs.
- Full CTest: 37/37 passed on 2026-08-09.

