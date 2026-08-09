# Cached state: JsonTools 3.2.0 evaluation

- Baseline: Notepad++ v8.4.6 plugin list, JsonTools tag `v3.2.0`, commit
  `7d9a94388adb8733959780d4a4d061c479a2a1d5`.
- Official x86/x64 ZIP hashes match the embedded catalogs. The x64 package
  contains only `JsonTools.dll`; its six standard exports were verified.
- Runtime: AMD64 PE32+, .NET Framework 4.0 (`v4.0.30319`), WinForms,
  `mscoree!_CorDllMain`, no plugin-private dependency DLL.
- Current Dock adapter and native host can be reused. No plugin architecture
  change or original window-tree emulation is required.
- Required host additions: current path, filename, file-new, do-open;
  `SCI_APPENDTEXT`, `SCI_GOTOLINE`, `SCI_GOTOPOS`; `NPPN_TBMODIFICATION` and
  `NPPN_FILEBEFORECLOSE`.
- `NPPN_TBMODIFICATION` is mandatory before commands run because NppPlugin.NET
  copies host command IDs back into its managed `FuncItems` list there.
- Windows recommendation: run the unchanged official DLL after the narrow
  adapter additions. Cross-platform recommendation: defer source/UI porting
  until the portable ABI is frozen.
- Implementation completed for loading, the narrow host API set, formatting,
  compression, notifications, and the first WinForms tree Dock path.
- JsonTools intentionally stores function index `4` in `tTbData::dlgID`; only
  menu check state uses the host command ID refreshed by `NPPN_TBMODIFICATION`.
- Deep validation now covers Settings, RemesPath query/assignment, JSON Lines,
  YAML, tree navigation, and the 4 MB partial/full-tree threshold. Upstream
  `Run tests` remains non-portable because JsonGrepper uses an unguarded author-absolute path.
- Compatibility grade remains B because it is Windows-only and still hosts
  the original CLR4/WinForms runtime.
- Detailed report:
  `codex/analysis/plugins-v846/jsontools-v320-compatibility-evaluation.md`.
