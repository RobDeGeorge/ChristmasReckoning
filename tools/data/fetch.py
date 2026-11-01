#!/usr/bin/env python3
"""Download the raw open datasets behind the Mission Control simulation.

Downloads go to ~/.cache/christmasreckoning/raw (outside the repository). Every file is pinned by
URL and SHA-256 in tools/data/sources.json; on first download the hash is recorded there, after
which mismatches are errors (so a silently changed upstream file can't alter the published numbers).
"""
from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import shutil
import sys
import urllib.request

HERE = pathlib.Path(__file__).resolve().parent
CACHE = pathlib.Path.home() / ".cache" / "christmasreckoning" / "raw"
SOURCES = HERE / "sources.json"


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def download(url: str, dest: pathlib.Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_name(dest.name + ".part")
    request = urllib.request.Request(url, headers={"User-Agent": "christmasreckoning-data/1.0"})
    with urllib.request.urlopen(request, timeout=300) as response, open(tmp, "wb") as out:
        total = int(response.headers.get("Content-Length") or 0)
        done = 0
        while True:
            block = response.read(1 << 20)
            if not block:
                break
            out.write(block)
            done += len(block)
            if total and sys.stderr.isatty():
                print(f"\r  {dest.name}: {done / total:6.1%}", end="", file=sys.stderr)
    if sys.stderr.isatty():
        print(file=sys.stderr)
    tmp.rename(dest)


def path_for(key: str) -> pathlib.Path:
    sources = json.loads(SOURCES.read_text())["datasets"]
    return CACHE / sources[key]["file"]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("keys", nargs="*", help="dataset keys to fetch (default: all)")
    args = parser.parse_args()

    manifest = json.loads(SOURCES.read_text())
    datasets = manifest["datasets"]
    changed = False
    for key, entry in datasets.items():
        if args.keys and key not in args.keys:
            continue
        dest = CACHE / entry["file"]
        if not dest.exists():
            print(f"downloading {key}: {entry['url']}", file=sys.stderr)
            download(entry["url"], dest)
        digest = sha256(dest)
        if not entry.get("sha256"):
            entry["sha256"] = digest
            changed = True
            print(f"  pinned {key} sha256={digest}", file=sys.stderr)
        elif entry["sha256"] != digest:
            print(f"error: {key} hash mismatch ({digest} != pinned {entry['sha256']})", file=sys.stderr)
            return 1
        print(f"ok  {key:<14} {dest.stat().st_size / 1048576:8.1f} MiB  {dest}")
    if changed:
        SOURCES.write_text(json.dumps(manifest, indent=2) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
