# 2026-08-09 Full Regression

## Scope

- Release build and the complete CTest suite.
- Notepad++ v8.4.6 and Qt UI comparison at 100% scale.
- Qt light/dark UI capture at 100% and 150% scale.
- Find dialog (5 pages) and Preferences (19 pages) interaction.
- Original XML corpus and current-user configuration round trips.
- Original v8.4.6 reading configuration written by the Qt port.

## Results

- Release build: passed.
- CTest: 37/37 passed in 42.83 seconds.
- Runtime capture: all page images were nonblank after allowing the original
  application to finish its lazy dialog initialization.
- Original readback: light and dark configurations written by the Qt port
  loaded without an XML error and selected the expected color mode.
- Qt 150% capture: no text overlap, clipped command buttons, or unusable page
  layout was observed.

## UI Matrix

| Case | Images | Main | Find | Preferences |
| --- | ---: | ---: | ---: | ---: |
| Original light 100% | 29 | 1100x760 | 589x364 | 846x411 |
| Original dark 100% | 29 | 1100x760 | 589x364 | 846x411 |
| Qt light 100% | 27 | 1100x760 | 589x375 | 846x411 |
| Qt dark 100% | 27 | 1100x760 | 589x375 | 846x411 |
| Qt light 150% | 27 | 1100x760 | 876x543 | 1261x597 |
| Qt dark 150% | 27 | 1100x760 | 876x543 | 1261x597 |

The original matrix has two additional images because Win32 automation also
captures Shortcut Mapper and Project Panel entry points. Their Qt counterparts
are covered by the programmatic UI parity test.

## Confirmed Differences

- The Qt Find dialog is 11 pixels taller at 100%; controls and command order
  remain behaviorally equivalent.
- Native Win32 and Qt theme engines draw tabs, check boxes, frames, status-bar
  separators, title bars, and toolbar spacing differently at the pixel level.
- The Qt toolbar icon set and spacing are close but not byte-for-byte identical
  to v8.4.6 Win32 resource rendering.
- These are presentation differences. No new command, navigation, localization,
  or configuration behavior regression was found.

## Fix Found During Regression

The Qt port previously ignored the original
`GUIConfig name="DarkMode" enable="..."` value and allowed the legacy
`qtState.ini` key `Editor/darkMode` to override it. The port now reads and
writes the original XML node, preserves all original color attributes, ignores
the legacy private key, and removes that key on the next state write.

## Evidence Location

Generated screenshots and isolated settings are under
`build/full-regression-2026-08-09/`. Build artifacts are intentionally not
tracked by Git.
