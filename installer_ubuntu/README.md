# Ubuntu Debian package

The Debian package keeps the executable, updater, localization files, and
function-list definitions together under `/opt/notepad-plus-plus`. A small
`/usr/bin/notepad++` launcher and desktop entry expose the application to the
desktop and command line.

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
