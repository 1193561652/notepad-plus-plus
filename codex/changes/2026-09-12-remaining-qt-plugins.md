# Remaining source-available Qt plugin integrations

The independently owned plugin repositories now provide CodeAlignment, DoxyIt,
Session Manager, PluginDemo, RandomValues, JsonTools and XMLTools on qt-port.
The shared workspace and exact repository commits live under tools/win32-plugins-qt.

PluginManager still owns plugin loading and ABI dispatch. PluginHostServices still
owns access to MainWindow/editor state. The only new host service is the current
lexer/UDL UTF-8 name, appended to NppPluginHostInfo; old struct prefixes remain valid.
DoxyIt uses this for original parser selection. The ABI fixture checks Unicode,
required size and truncated NUL termination. The host cross-platform-plugin-tests
passed when run with -platform windows. The default CTest backend timed out
after 30 seconds on the final rerun; the explicit Windows backend exited 0 with
abi-ready/abi-shutdown. The entire host regression suite is not claimed to pass.

Original parser/generator/query/XML code remains inside each plugin's repository.
RandomValues and JsonTools run the original C# cores on Windows Framework 4.8,
with Qt UI communicating over a persistent process. XMLTools directly compiles its
three formatters and original MSXML wrapper/helper in an isolated MSVC backend.

The packaged workspace currently registers 51 tests. In the 50-test integration
run, 45 passed and five Windows OLE clipboard tests failed, including outside the
sandbox. The additional MSXML test passed. Offscreen retests passed JsonTools,
SelectToClipboard and converter; BetterMultiSelection remained unsuccessful there.
Original JsonTools suites retain three YAML baseline failures. See PORTING_STATUS.md
for detailed limitations, unverified platforms and the seven source-unavailable
plugins explicitly skipped by the user. Installation into build-qt/package passed.
