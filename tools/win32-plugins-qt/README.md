# Independently versioned Qt plugins

The plugin implementations are maintained in their existing repositories.
`repositories.json` pins the commits used by this host revision. `workspace/`
tracks the shared CMake entry point, ABI adapters, tests and porting record that
are deployed into the sibling `win32-plugins` directory.

From the host repository, restore that workspace with:

```sh
python tools/win32-plugins-qt/prepare.py ../win32-plugins
```

The script clones missing repositories at the recorded commits. It preserves
existing repositories at different revisions and differing integration files by
stopping with an error. It does not reset, clean or discard edits.
See the restored `qt/README.md` for the pinned EditorConfig/RapidJSON dependency
setup and build commands. A different host location can be supplied through
`NPP_QT_HOST_SOURCE` when configuring CMake.

When continuing development, commit each plugin separately, update its revision
in the manifest, and copy changes to shared integration files back into
`workspace/` before committing this repository. Build products, downloaded
dependencies and local runtime settings are intentionally excluded.
