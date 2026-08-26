# Original resource loading alignment

The Qt port now carries the complete `PowerEditor/src/icons` tree from the
Notepad++ 8.4.6 baseline. CMake generates a Qt resource manifest from that
tree, so the executable embeds the upstream directory structure without a
second hand-maintained icon list.

Toolbar selection follows the original `config.xml` contract:

- `small` / `large`: Fluent UI icons at 16 / 32 pixels;
- `small2` / `large2`: filled Fluent UI icons at 16 / 32 pixels;
- `standard`: the original 16-pixel colour bitmap set;
- dark mode selects the corresponding upstream dark resource directory;
- upstream disabled icons are loaded where supplied;
- `toolbarIcons.xml` overrides only files present in the selected custom set,
  with built-in icons and disabled states retained for missing files;
- a custom icon set makes `standard` use the small Fluent base, matching the
  original fallback behaviour.

The toolbar status is independent from the tab icon-set setting and is read
and written using the same values as upstream. Preferences reapply resources
immediately after toolbar or dark-mode changes.

Document tabs also select the upstream normal, alternate, or dark icon tree at
runtime and use the original saved, modified, read-only, and monitoring state
precedence. The tab bar paints the original 11-pixel close-button bitmaps for
normal, inactive, hover, and pressed states, including the upstream dark
variants. Qt's platform close-tab button is disabled so its theme cannot
silently replace those resources.

The remaining panel consumers now use the same upstream selection rules:

- dock tabs use the standard, `*2.ico`, or dark panel icon according to the
  toolbar status and dark mode;
- Folder as Workspace and Function List load their original light/dark toolbar
  bitmaps;
- project and file trees load the original workspace, project, folder, file,
  and invalid-file bitmaps;
- Document List reuses the same saved/modified/read-only/monitoring precedence
  as document tabs;
- About uses the original chameleon icon and its dark variant.

`UiResourceLoader` centralizes the Win32-compatible bitmap policies. Standard
toolbar and panel toolbars use the bitmap's top-left colour as the transparent
key, tree image lists use the original RGB(192,192,192) mask, and tab-close
bitmaps remain opaque to match upstream `SRCCOPY`. Requested logical sizes are
scaled through Qt so the same implementation remains DPI-aware on Windows,
Ubuntu, and macOS.

Qt continues to use each operating system's native widget and window-manager
chrome. Those platform controls are not Notepad++ image resources and are
intentionally not replaced with Windows-only rendering.
