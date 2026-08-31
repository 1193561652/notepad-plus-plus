# Ubuntu Debian package

The Debian package follows the normal Ubuntu filesystem layout:

- `/usr/bin/notepad++`: application executable;
- `/usr/libexec/notepad-plus-plus-qt/npp-plugin-updater`: helper executable;
- `/usr/share/notepad-plus-plus-qt/`: localization and function-list data;
- `/usr/share/applications/` and `/usr/share/icons/`: desktop integration.

The runtime path resolver still checks the executable directory first, which
keeps the original Windows and portable Notepad++ resource layout compatible.

Build and test the package from the repository root:

```bash
cmake -S . -B build-ubuntu-package \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON
cmake --build build-ubuntu-package -j "$(nproc)"
ctest --test-dir build-ubuntu-package --output-on-failure
cpack --config build-ubuntu-package/CPackConfig.cmake -G DEB
```

Inspect without installing:

```bash
dpkg-deb --info notepad-plus-plus-qt_*.deb
dpkg-deb --contents notepad-plus-plus-qt_*.deb
```
