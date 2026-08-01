# QScintilla to Scintilla 5 Qt migration plan

Date: 2026-07-30

Status: completed on 2026-08-01

Implementation result:

- Imported Scintilla 5.3.0 and Lexilla 5.1.9 from the v8.4.6 baseline.
- `ScintillaEditView` now derives from official `ScintillaEditBase`.
- The application uses real `ILexer5` and `CreateLexer -> SCI_SETILEXER`.
- Scintilla, Lexilla, and the original Boost.Regex adapter build as static
  CMake targets.
- QScintilla sources, includes, classes, and qmake build scripts were removed.
- Windows/MinGW full build and CTest `29/29` passed.

Decision record:
`codex/decisions/2026-07-30-scintilla5-qt-baseline.md`

## 1. Goal

Replace QScintilla 2.13.3 and its Scintilla 3.10.1-derived core with the
official Scintilla 5 Qt implementation included in Notepad++ v8.4.6, while
keeping application behavior and call structure as close to the original as
possible.

This is an editor-core migration, not an editor UI redesign. Existing
Notepad++ configuration, command, Buffer, session, search, and view behavior
must remain above a narrow `ScintillaEditView` platform adapter.

## 2. Verified source baseline

The local v8.4.6 reference contains:

| Component | Version | Reference source |
| --- | --- | --- |
| Scintilla | 5.3.0 (`530`) | `notepad-plus-plus-v8.4.6/scintilla/` |
| Scintilla Qt platform | 5.3.0 | `scintilla/qt/ScintillaEditBase/` |
| Optional generated Qt wrapper | 5.3.0 | `scintilla/qt/ScintillaEdit/` |
| Lexilla | 5.1.9 (`519`) | `notepad-plus-plus-v8.4.6/lexilla/` |
| Boost.Regex adapter | v8.4.6 baseline | `boostregex/BoostRegExSearch.cxx` |

The original executable statically links `libscintilla.lib` and
`liblexilla.lib`. It calls `CreateLexer(name)`, sends the returned
`ILexer5*` through `SCI_SETILEXER`, and lets Scintilla own the instance.

## 3. Recommended widget choice

Use `ScintillaEditBase`, not the generated `ScintillaEdit` convenience API, as
the direct base of the project adapter.

Reasons:

- `ScintillaEditBase::send()` exposes the raw Scintilla message contract.
- Original Notepad++ business code is organized around `execute(SCI_*, ...)`.
- Avoiding hundreds of generated convenience methods keeps the adapter small.
- The base widget already implements Qt painting, scrolling, clipboard,
  drag/drop, input method handling, and Scintilla notification signals.

Target declaration:

```cpp
class ScintillaEditView : public ScintillaEditBase {
    Q_OBJECT
public:
    sptr_t execute(
        unsigned int message,
        uptr_t wParam = 0,
        sptr_t lParam = 0) const;
};
```

Use pointer-width Scintilla types throughout this boundary. Do not retain
`long` for message results or pointer parameters.

## 4. Target source and build layout

Planned repository layout:

```text
third_party/
  scintilla/
    version.txt
    include/
    src/
    qt/ScintillaEditBase/
    LICENSE
    ORIGIN.md
  lexilla/
    version.txt
    include/
    lexers/
    lexlib/
    src/
    LICENSE
    ORIGIN.md
  boostregex/
```

Planned CMake targets:

| Target | Contents |
| --- | --- |
| `npp-scintilla-qt` | Scintilla 5.3.0 core, official Qt platform, `SCI_OWNREGEX` adapter |
| `npp-lexilla` | Exact Lexilla 5.1.9 catalogue, lexlib, built-in and N++ lexers |
| `notepadpp-qt` | Application and `ScintillaEditView`, links both static libraries |

Required build properties:

- `SCINTILLA_QT`
- `SCI_OWNREGEX`
- C++17
- Qt Core, Gui, and Widgets
- Qt Core5Compat only when building the optional generated wrapper on Qt 6;
  it is not expected to be needed by the recommended base-widget route.

Lexers belong only to the independent Lexilla target and must not be compiled
into the editor core. The original Windows project also defines
`SCI_EMPTYCATALOGUE`; in this source baseline the Scintilla 5 core is already
separated from the Lexilla catalogue.

## 5. Migration phases

### Phase 0: Freeze the QScintilla baseline

1. Record the current full build and CTest result.
2. Preserve current UI/runtime captures for comparison.
3. Inventory direct QScintilla dependencies.
4. Prevent unrelated feature work from being mixed into the core switch.

Current inventory: 16 source/test files directly reference `Qsci*` symbols.
The majority of editor behavior already uses raw `SCI_*` messages.

Exit gate:

- Current branch builds from an empty build directory.
- Full CTest and key runtime captures are reproducible.

### Phase 1: Import exact upstream sources

1. Import Scintilla 5.3.0 from the v8.4.6 reference.
2. Replace the adapted old-ABI Lexilla tree with exact Lexilla 5.1.9.
3. Preserve `LexUser.cxx` and `LexSearchResult.cxx` from that Lexilla tree.
4. Add `ORIGIN.md` files containing source tag, version, copied paths, and
   checksums or a file manifest.
5. Build a standalone Scintilla Qt smoke-test executable.
6. Build a standalone Lexilla ABI test against Scintilla 5 headers.

The old and new Scintilla libraries may coexist in the repository during this
phase, but they must not be linked into the same executable because they
define overlapping Scintilla symbols and incompatible lexer ABIs.

Exit gate:

- `npp-scintilla-qt` builds on Windows and Linux.
- `npp-lexilla` returns real `ILexer5` instances with `lvRelease5`.
- The smoke-test widget displays and edits UTF-8 text.

### Phase 2: Establish the application adapter

1. Change `ScintillaEditView` to derive from `ScintillaEditBase`.
2. Implement `execute()` as a thin call to `send()`.
3. Add narrowly scoped helper methods only for behavior currently supplied by
   QScintilla and used by the application.
4. Convert message parameters and document pointers to `sptr_t`, `uptr_t`,
   `Position`, and `Document`-appropriate pointer-width types.
5. Keep existing public application-facing method names where this avoids
   unrelated MainWindow changes.

First helper set:

- text get/set/append/insert;
- cursor and selection;
- EOL, wrapping, whitespace, indentation guides, zoom;
- margins, markers, indicators, folding;
- autocompletion and call tips;
- UTF-8 conversion at the Qt boundary.

Exit gate:

- A minimal application target creates `ScintillaEditView`.
- Basic editing, undo/redo, selection, EOL and styling tests pass.

### Phase 3: Map notifications and Qt events

Map official `ScintillaEditBase` signals to the existing application slots:

| Scintilla/Qt signal | Existing behavior |
| --- | --- |
| `charAdded` | auto-pair and autocompletion |
| `updateUi` | smart highlight and status refresh |
| `modified` | dirty state and Buffer updates |
| `savePointChanged` | tab dirty marker |
| `marginClicked` | folding and bookmarks |
| `hotSpotClick` | URL opening |
| `doubleClick` | Find Result navigation |
| `macroRecord` | macro command recording |
| `focusChanged` | active view synchronization |

Keep translation code in `ScintillaEditView`; business modules should not
depend on Qt-platform Scintilla internals.

Exit gate:

- Notification ordering and arguments match the current behavior tests.
- No feature polls editor state solely to replace a notification that exists.

### Phase 4: Replace QsciLexer with original Lexilla flow

1. Remove QsciLexer classes from language selection.
2. Resolve the lexer name from Notepad++ language/configuration data.
3. Call Lexilla 5.1.9 `CreateLexer(name)`.
4. Send the `ILexer5*` using `SCI_SETILEXER`.
5. Apply properties, keywords and style IDs through `SCI_SETPROPERTY`,
   `SCI_SETKEYWORDS`, and `SCI_STYLE*`, following original ordering.
6. Keep `stylers.xml`, `langs.xml`, UDL and search-result behavior as the
   application-level sources of configuration.

Do not reproduce QsciLexer as another permanent metadata hierarchy. Where
Qt-side metadata is still needed, use a small non-owning language descriptor
derived from the Notepad++ XML model.

Exit gate:

- Built-in languages, UDL, search-result and external-lexer-ready paths use
  real `ILexer5`.
- Lexer replacement and destruction pass ownership/leak tests.
- Representative style/fold output matches v8.4.6.

### Phase 5: Replace document and dual-view abstractions

Replace `QsciDocument` with original Scintilla document messages:

- `SCI_CREATEDOCUMENT`
- `SCI_ADDREFDOCUMENT`
- `SCI_RELEASEDOCUMENT`
- `SCI_GETDOCPOINTER`
- `SCI_SETDOCPOINTER`

Preserve Notepad++ Buffer ownership above these calls. Validate:

- one Buffer in one view;
- clone in main and sub view;
- moving between views;
- closing one clone without destroying the shared document;
- invisible/scratch view operations;
- session restore positions and folds;
- large-file document options.

Exit gate:

- Document reference counts survive all open/move/clone/close sequences.
- Existing Buffer, dual-view, session and large-file tests pass.

### Phase 6: Remove remaining QScintilla conveniences

Replace the remaining types:

| QScintilla type | Replacement |
| --- | --- |
| `QsciAPIs` | existing `autoCompletion/*.xml` model plus `SCI_AUTOC*` |
| `QsciPrinter` | current Qt PrintSupport adapter using Scintilla formatting |
| `QsciMacro` | project macro recorder driven by `SCN_MACRORECORD` |
| `QsciCommand` | Notepad++ shortcut model and direct Scintilla key commands |
| Qsci enums | official `ScintillaTypes.h`, `ScintillaMessages.h`, `SciLexer.h` |

Update all tests to instantiate `ScintillaEditView` or a minimal
`ScintillaEditBase` harness rather than `QsciScintilla`.

Exit gate:

- No `#include <Qsci/...>` or `Qsci*` symbol remains in application/tests.
- The application no longer links QScintilla.

### Phase 7: Delete the old core and close the migration

1. Remove `third_party/qscintilla/`, its qmake bridge and imported CMake target.
2. Remove the old-ABI `SCI_SETILEXER` compatibility implementation.
3. Rename build documentation and targets to Scintilla/Lexilla terminology.
4. Run clean Windows and Linux builds.
5. Run the complete automated and manual editor parity matrix.
6. Update all affected `codex/` indexes and close this plan with a change
   record.

Exit gate:

- Only Scintilla 5.3.0 headers and symbols are present.
- Only Lexilla 5.1.9 lexer ABI is present.
- Full CTest passes from an empty build tree.
- Approved runtime checks show no blocking editor/UI regression.

## 6. Validation matrix

Automated coverage must include:

| Area | Required checks |
| --- | --- |
| Core | text, UTF-8, undo/redo, selection, rectangular/multiple selection |
| EOL/encoding | CRLF/LF/CR, BOM, code-page conversion, save/reload |
| Regex | Boost.Regex search, replacement, zero-length, backward search |
| Lexer | C++, XML, JSON, UDL, searchResult, properties, keywords, folding |
| Buffer | create, switch, clone, move, close, shared-document lifetime |
| Large file | document options, chunk load, feature downgrade |
| UI events | margin, hotspot, update UI, dirty state, macro recording |
| Qt platform | clipboard, drag/drop, IME, wheel, high DPI, dark/light palette |
| Features | Find Result, document map, function list, autocompletion, printing |

Platform matrix:

- Windows: Qt 5.12.12, MinGW; add MSVC when available.
- Linux: Qt 5.15.x, GCC/Clang.
- macOS: build and runtime validation when a test host is available.

Manual UI checks remain a final gate for caret rendering, font metrics,
scrolling, IME, drag/drop, menus, Find Result interaction and dual-view focus.

## 7. Main risks and controls

| Risk | Control |
| --- | --- |
| C++14 to C++17 change | make it an explicit core-migration prerequisite; do not patch upstream back to C++14 |
| Old/new symbol collision | never link both Scintilla generations into one executable |
| Notification argument differences | central translation in `ScintillaEditView` plus signal-order tests |
| QsciLexer behavior loss | port by original lexer name/property/keyword order, not by class substitution |
| Document lifetime regression | reference-count tests for every main/sub/clone transition |
| Qt 5.12 platform differences | compile exact Qt source early and keep platform fixes isolated and documented |
| Regex semantic regression | compile the v8.4.6 Boost adapter into Scintilla with `SCI_OWNREGEX` |
| Upstream source drift | pin version files and manifests; no latest-version substitution |
| Large change size | enforce phase exit gates and keep feature work out of the migration |

## 8. Rollback strategy

Each phase must remain independently reviewable. Until Phase 7:

- preserve the QScintilla baseline build path;
- use separate smoke-test executables for the new core;
- switch the main executable only after adapter, lexer and document tests pass;
- do not delete QScintilla until the main executable passes the full matrix.

Rollback means selecting the previous editor target at the build boundary, not
mixing old and new ABI objects in one process.
