# Original Notepad++ Preferences Lag Diagnosis

Date: 2026-07-25

## Observed behavior

- Opening the original Notepad++ Preferences dialog is slow.
- Dragging the Preferences window has visible latency.
- The behavior was reproduced with the installed v7.81 executable.
- The behavior was also reproduced with a locally built v8.4.6 `Release|Win32` executable.

## Isolation results

The lag still occurs with all of the following controls:

- `-noPlugin`
- `-nosession`
- an absent `plugins/` directory
- portable mode via `doLocalConf.xml`
- pristine v8.4.6 `config.4zipPackage.xml`
- no access to the shared `%APPDATA%\Notepad++` configuration

Therefore the lag is not caused by the Qt port's earlier writes to shared
`config.xml` or `nativeLang.xml`. It is also not caused by a loaded plugin or
the plugin directory scan.

Earlier plugin comparisons used the installed v7.81 executable copied into
diagnostic directories. They were not valid v8.4.6 performance baselines.
The final v8.4.6 result used a clean Release build from
`v8.4.6:PowerEditor/bin/notepad++.exe`.

## Relevant original implementation

- `PreferenceDlg::doDialog()` creates the Preferences dialog on first use.
- The original implementation creates the main dialog and approximately 19
  child preference pages during first initialization.
- `StaticDialog::create()` registers these windows as modeless dialogs.
- Closing Preferences hides the dialog instead of destroying its pages.
- The main message loop subsequently checks registered modeless dialogs for
  each normal Windows message.

Relevant source:

- `PowerEditor/src/WinControls/Preference/preferenceDlg.cpp`
- `PowerEditor/src/WinControls/StaticDialog/StaticDialog.cpp`
- `PowerEditor/src/Notepad_plus_Window.cpp`
- `PowerEditor/src/winmain.cpp`

This design is a plausible source of the original dialog's first-open and
interactive overhead on the current Windows environment. It is independent of
the Qt port configuration compatibility work.

## Port guidance

- Do not change shared configuration again to address this lag.
- Keep the Qt Preferences implementation behavior-compatible, but avoid
  reproducing the original modeless-dialog message-loop overhead where Qt
  lifecycle and page switching can provide the same visible behavior.
- Compare UI content and state against the original, not the original's
  performance defect.
