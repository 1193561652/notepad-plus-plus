#!/usr/bin/env python3
"""Restore the independently versioned Qt plugin workspace without overwriting edits."""
import argparse
import json
import shutil
import subprocess
from pathlib import Path


def git(path, *args):
    return subprocess.check_output(
        ["git", "-C", str(path), *args], text=True, encoding="utf-8"
    ).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path, help="Destination, normally ../win32-plugins")
    args = parser.parse_args()
    source = Path(__file__).resolve().parent
    target = args.directory.resolve()
    manifest = json.loads((source / "repositories.json").read_text(encoding="utf-8"))
    payload = source / "workspace"
    files = [p for p in payload.rglob("*") if p.is_file()]
    # Inspect everything first. Existing worktrees and differing files are preserved.
    for entry in manifest:
        repository = target / entry["directory"]
        if repository.exists():
            if git(repository, "rev-parse", "HEAD") != entry["revision"]:
                raise SystemExit(f"Different revision in {repository}; existing checkout preserved")
            if git(repository, "branch", "--show-current") != entry["branch"]:
                raise SystemExit(f"Switch {repository} to {entry['branch']} before preparing the workspace")
    for path in files:
        destination = target / path.relative_to(payload)
        if destination.exists() and destination.read_bytes() != path.read_bytes():
            raise SystemExit(f"Different file: {destination}; existing file preserved")
    target.mkdir(parents=True, exist_ok=True)
    for entry in manifest:
        repository = target / entry["directory"]
        if not repository.exists():
            subprocess.run(["git", "clone", "--no-checkout", "--branch", entry["branch"], entry["url"], str(repository)], check=True)
            # This is a newly created clone, so there is no existing local work to replace.
            subprocess.run(["git", "-C", str(repository), "checkout", "-B", entry["branch"], entry["revision"]], check=True)
    for path in files:
        destination = target / path.relative_to(payload)
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, destination)
    print(f"Prepared {target}. Follow qt/README.md to prepare dependencies and build.")


if __name__ == "__main__":
    main()
