#!/usr/bin/env python3
# Notepad++ for Qt native-language resource synchronizer.
# Copyright (C) 2026 Jiang Liwei.
# Distributed under the Notepad++ GNU GPL v3 terms, clarifications and
# exceptions in the repository's LICENSE file.
"""Synchronize all unmodified v8.4.6 native-language XML resources."""

from __future__ import annotations

import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BASELINE = "v8.4.6"
UPSTREAM_DIR = "PowerEditor/installer/nativeLang"


def git_output(*arguments: str) -> bytes:
    return subprocess.check_output(["git", *arguments], cwd=ROOT)


def upstream_filenames() -> list[str]:
    paths = git_output(
        "ls-tree", "-r", "--name-only", BASELINE, UPSTREAM_DIR
    ).decode("utf-8").splitlines()
    return sorted(Path(path).name for path in paths if path.endswith(".xml"))


def main() -> int:
    output_dir = ROOT / "installer_common/nativeLang"
    output_dir.mkdir(parents=True, exist_ok=True)
    filenames = upstream_filenames()
    if len(filenames) != 94:
        raise RuntimeError(
            f"expected 94 v8.4.6 language resources, found {len(filenames)}"
        )

    for filename in filenames:
        data = git_output("show", f"{BASELINE}:{UPSTREAM_DIR}/{filename}")
        destination = output_dir / filename
        destination.write_bytes(data)
        if destination.read_bytes() != data:
            raise RuntimeError(f"failed to reproduce upstream {filename}")

    print(f"Synchronized {len(filenames)} language files from {BASELINE}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
