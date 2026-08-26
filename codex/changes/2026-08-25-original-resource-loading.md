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
precedence.

Qt continues to use each operating system's native widget and window-manager
chrome. Those platform controls are not Notepad++ image resources and are
intentionally not replaced with Windows-only rendering.
