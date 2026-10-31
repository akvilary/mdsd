#!/usr/bin/env bash
# Build and install mdsd. Works from any directory: artifacts always land
# in the project directory (same as the script itself).
# Minimum supported standard: C++20.
#
#   bash install.sh              — build + install (root: /usr/local; otherwise ~/.local)
#   bash install.sh build        — build only, place ./mdsd at project root
#   bash install.sh test         — build and run unit tests (ctest)
#   bash install.sh install      — explicit install
#   bash install.sh uninstall    — remove installed files (via install_manifest.txt)
#   bash install.sh clean        — remove build/
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
CMD="${1:-install}"

pick_compiler() {
  if [[ -n "${CXX:-}" ]]; then
    echo "$CXX"
    return
  fi
  # C++20: any modern compiler works; prefer newer versions
  local c
  for c in g++-14 g++-13 g++-12 clang++-18 clang++-17 clang++-16 clang++-15 g++; do
    if command -v "$c" >/dev/null 2>&1; then
      echo "$c"
      return
    fi
  done
  echo "g++"
}

do_build() {
  local cxx
  cxx="$(pick_compiler)"
  local jobs
  jobs="$(nproc 2>/dev/null || echo 4)"
  echo "==> Compiler: $cxx"
  cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="$cxx" >/dev/null
  cmake --build "$BUILD_DIR" -j "$jobs"
  cp -f "$BUILD_DIR/mdsd" "$SCRIPT_DIR/mdsd"
  chmod +x "$SCRIPT_DIR/mdsd"
  echo "==> Done: $SCRIPT_DIR/mdsd"
}

case "$CMD" in
  build)
    do_build
    ;;
  test)
    do_build
    ctest --test-dir "$BUILD_DIR" --output-on-failure
    ;;
  clean)
    rm -rf "$BUILD_DIR"
    echo "==> build/ removed"
    ;;
  install)
    do_build
    DEST="${PREFIX:-}"
    if [[ -z "$DEST" ]]; then
      if [[ ${EUID:-$(id -u)} -eq 0 ]]; then
        DEST="/usr/local"
      else
        DEST="$HOME/.local"
      fi
    fi
    cmake --install "$BUILD_DIR" --prefix "$DEST"
    if [[ "$DEST" == "$HOME/.local" && ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
      echo "==> Note: \$HOME/.local/bin is not in PATH; add: export PATH=\"\$HOME/.local/bin:\$PATH\""
    else
      echo "==> Installed to $DEST (command: mdsd)"
    fi
    ;;
  uninstall)
    MANIFEST="$BUILD_DIR/install_manifest.txt"
    if [[ ! -f "$MANIFEST" ]]; then
      echo "Error: $MANIFEST not found — run 'bash install.sh install' first" >&2
      exit 1
    fi
    while IFS= read -r f; do
      [[ -f "$f" ]] && rm -f "$f" && echo "removed: $f"
    done < "$MANIFEST"
    ;;
  *)
    echo "Unknown command: $CMD (supported: build | test | install | uninstall | clean)" >&2
    exit 2
    ;;
esac
