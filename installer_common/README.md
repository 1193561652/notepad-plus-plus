# Shared installer resources

This directory contains metadata and install rules shared by the Windows,
Ubuntu, and future macOS packages. Platform-specific installer logic remains
in `installer/`, `installer_ubuntu/`, and `installer_mac/`.

The official language payload stays in `installer_common/nativeLang/`, which
is the single source used by builds and every installer.
`LocalizationComponents.cmake` defines how that shared payload is grouped for
a target package.
