#!/usr/bin/env python3
"""Assemble the deployable site from a WebAssembly build.

Copies the web shell, renders index.html, and writes content-hashed app files so GitHub Pages'
10-minute cache can never pair an old .js with a new .wasm. The .wasm is also precompressed
(brotli and gzip) because static hosts don't reliably compress it; web/loader.js streams whichever
variant the browser can decode.
"""
from __future__ import annotations

import argparse
import gzip
import hashlib
import html
import json
import pathlib
import shutil
import sys

REPO = pathlib.Path(__file__).resolve().parents[2]
SOURCE_URL = "https://github.com/RobDeGeorge/ChristmasReckoning"


def digest(*paths: pathlib.Path) -> str:
    h = hashlib.sha256()
    for path in paths:
        h.update(path.read_bytes())
    return h.hexdigest()[:12]


def brotli_compress(data: bytes) -> bytes | None:
    try:
        import brotli  # type: ignore
    except ImportError:
        return None
    return brotli.compress(data, quality=11, lgwin=24)


def render_summary(path: pathlib.Path | None) -> str:
    """Static HTML table of headline numbers from cr-report (empty until that tool exists)."""
    if not path or not path.exists():
        return ""
    report = json.loads(path.read_text())
    rows = "\n".join(
        f"<tr><th scope=\"row\">{html.escape(item['label'])}</th><td>{html.escape(item['value'])}</td></tr>"
        for item in report.get("headline", []))
    intro = html.escape(report.get("intro", ""))
    return f"<h2>Santa's night, by the numbers</h2>\n<p>{intro}</p>\n<table>\n{rows}\n</table>"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=pathlib.Path,
                        default=pathlib.Path.home() / "build/christmasreckoning/wasm-release")
    parser.add_argument("--out", type=pathlib.Path,
                        default=pathlib.Path.home() / "build/christmasreckoning/dist")
    parser.add_argument("--summary", type=pathlib.Path, help="cr-report JSON for the static summary")
    args = parser.parse_args()

    app_dir = args.build / "src" / "app"
    wasm = app_dir / "christmasreckoning.wasm"
    app_js = app_dir / "christmasreckoning.js"
    qtloader = app_dir / "qtloader.js"
    for path in (wasm, app_js, qtloader):
        if not path.exists():
            print(f"missing build output: {path}", file=sys.stderr)
            return 1

    out = args.out
    if out.exists():
        shutil.rmtree(out)
    shutil.copytree(REPO / "web", out, ignore=shutil.ignore_patterns("index.html", "*.md"))
    shutil.copy2(REPO / "CNAME", out / "CNAME")

    app_hash = digest(wasm, app_js)
    names = {
        "js": f"app.{app_hash}.js",
        "wasm": f"app.{app_hash}.wasm",
        "wasmGz": f"app.{app_hash}.wasm.gz",
        "wasmBr": f"app.{app_hash}.wasm.br",
        "qtloader": f"qtloader.{digest(qtloader)}.js",
    }
    shutil.copy2(app_js, out / names["js"])
    shutil.copy2(qtloader, out / names["qtloader"])
    wasm_bytes = wasm.read_bytes()
    (out / names["wasm"]).write_bytes(wasm_bytes)
    gz = gzip.compress(wasm_bytes, compresslevel=9, mtime=0)
    (out / names["wasmGz"]).write_bytes(gz)
    br = brotli_compress(wasm_bytes)
    if br is not None:
        (out / names["wasmBr"]).write_bytes(br)

    build = {
        "entry": "christmasreckoning_entry",
        "wasm": names["wasm"],
        "wasmBytes": len(wasm_bytes),
        "wasmGz": names["wasmGz"],
        "wasmGzBytes": len(gz),
        "wasmBr": names["wasmBr"] if br is not None else None,
        "wasmBrBytes": len(br) if br is not None else None,
    }
    template = (REPO / "web" / "index.html").read_text()
    page = (template
            .replace("{{BUILD_JSON}}", json.dumps(build, separators=(",", ":")))
            .replace("{{APP_JS}}", names["js"])
            .replace("{{QTLOADER_JS}}", names["qtloader"])
            .replace("{{SOURCE_URL}}", SOURCE_URL)
            .replace("{{SUMMARY}}", render_summary(args.summary)))
    if "{{" in page:
        print("index.html still has unrendered placeholders", file=sys.stderr)
        return 1
    (out / "index.html").write_text(page)

    def kib(n: int) -> str:
        return f"{n / 1024:,.0f} KiB"

    info = {
        "hash": app_hash,
        "wasm": len(wasm_bytes),
        "wasmGz": len(gz),
        "wasmBr": len(br) if br is not None else None,
        "js": (out / names["js"]).stat().st_size,
        "textures": sum(p.stat().st_size for p in (out / "textures").glob("*")),
    }
    (out / "build-info.json").write_text(json.dumps(info, indent=2) + "\n")
    print(f"site written to {out}")
    print(f"  wasm {kib(info['wasm'])} · gzip {kib(info['wasmGz'])}"
          + (f" · brotli {kib(info['wasmBr'])}" if info["wasmBr"] else "")
          + f" · js {kib(info['js'])} · textures {kib(info['textures'])}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
