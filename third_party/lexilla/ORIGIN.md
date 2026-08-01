# Lexilla Source Origin

- Upstream baseline: Lexilla 5.1.9 (`version.txt` = `519`).
- Project baseline: Notepad++ v8.4.6.
- Imported from: `notepad-plus-plus-v8.4.6/lexilla/`.
- Integration: lexlib, the catalogue, built-in lexers, `LexUser.cxx`, and
  `LexSearchResult.cxx` are compiled into the static CMake target
  `npp-lexilla`.
- Portable adaptation: `LexUser.cxx` imports the `Lexilla` namespace
  explicitly for GCC/MinGW compilation.

The application creates `ILexer5` instances with `CreateLexer()` and installs
them through `SCI_SETILEXER`, matching the original Notepad++ call direction.
