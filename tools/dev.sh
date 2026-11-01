#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Developer shortcuts. Build output and caches stay outside the (Dropbox-synced) repository.
#
#   tools/dev.sh native     configure + build the desktop app and tests (Debug)
#   tools/dev.sh test       run the unit tests
#   tools/dev.sh run        run the desktop app
#   tools/dev.sh wasm       configure + build the WebAssembly app (MinSizeRel)
#   tools/dev.sh package    assemble the deployable site into ~/build/christmasreckoning/dist
#   tools/dev.sh serve      serve the packaged site on http://localhost:8000
#   tools/dev.sh smoke      run the headless-browser smoke test against the packaged site
#   tools/dev.sh all        native + test + wasm + package + smoke
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${CR_BUILD_ROOT:-$HOME/build/christmasreckoning}"
CACHE="${CR_CACHE:-$HOME/.cache/christmasreckoning}"
PYTHON="${CR_PYTHON:-$CACHE/venv/bin/python}"
[ -x "$PYTHON" ] || PYTHON=python3
cd "$ROOT"

emsdk_env() {
    # shellcheck disable=SC1091
    source "${EMSDK:-$HOME/emsdk}/emsdk_env.sh" >/dev/null 2>&1
}

native()  { cmake --preset native-debug && cmake --build --preset native-debug; }
tests()   { ctest --preset native-debug; }
run()     { "$BUILD/native-debug/src/app/christmasreckoning" "$@"; }
wasm()    { emsdk_env && cmake --preset wasm-release && cmake --build --preset wasm-release; }
report()  {
    if [ -x "$BUILD/native-debug/src/tools/cr-report" ]; then
        "$BUILD/native-debug/src/tools/cr-report" > "$BUILD/summary.json"
        echo "--summary $BUILD/summary.json"
    fi
}
package() { "$PYTHON" tools/web/package.py --build "$BUILD/wasm-release" --out "$BUILD/dist" $(report); }
serve()   { "$PYTHON" -m http.server --directory "$BUILD/dist" 8000; }
smoke()   { NODE_PATH="$CACHE/node/node_modules" node tools/smoke/smoke.cjs --dist "$BUILD/dist" --out "$BUILD/smoke" "$@"; }

cmd="${1:-all}"
shift || true
case "$cmd" in
    native) native ;;
    test) tests ;;
    run) run "$@" ;;
    wasm) wasm ;;
    package) package ;;
    serve) serve ;;
    smoke) smoke "$@" ;;
    all) native && tests && wasm && package && smoke ;;
    *) echo "unknown command: $cmd" >&2; exit 2 ;;
esac
