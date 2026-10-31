#!/usr/bin/env python3
"""Cross-platform build and install script for mdsd.

Works on Linux, macOS, and Windows. Requires: Python 3.8+, CMake 3.16+,
and a C++20 compiler.

Usage:
    python3 install.py              # build + install
    python3 install.py build        # build only, place ./mdsd at project root
    python3 install.py test         # build and run unit tests
    python3 install.py install      # explicit install
    python3 install.py uninstall    # remove installed files
    python3 install.py clean        # remove build/
"""

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
BUILD_DIR = SCRIPT_DIR / "build"
BINARY_NAME = "mdsd.exe" if platform.system() == "Windows" else "mdsd"
BINARY_PATH = SCRIPT_DIR / BINARY_NAME


def log(msg: str) -> None:
    print(f"==> {msg}")


def die(msg: str) -> None:
    print(f"Error: {msg}", file=sys.stderr)
    sys.exit(1)


def run(cmd: list[str], **kwargs) -> subprocess.CompletedProcess:
    """Runs a command; dies on failure."""
    result = subprocess.run(cmd, **kwargs)
    if result.returncode != 0:
        die(f"command failed ({result.returncode}): {' '.join(cmd)}")
    return result


def pick_compiler() -> str | None:
    """Finds the best available C++20 compiler."""
    # User override via CXX environment variable
    if cxx := os.environ.get("CXX"):
        if shutil.which(cxx):
            return cxx
        die(f"CXX={cxx} not found in PATH")

    system = platform.system()

    if system == "Windows":
        # Let CMake auto-detect MSVC (Visual Studio generator) or clang
        return None

    if system == "Darwin":
        # macOS: prefer clang++ (Apple Clang or Homebrew)
        for c in ("clang++", "g++-14", "g++-13", "g++"):
            if shutil.which(c):
                return c
        return None

    # Linux: prefer newer g++, then clang++
    for c in ("g++-14", "g++-13", "g++-12", "clang++-18", "clang++-17",
              "clang++-16", "clang++-15", "g++"):
        if shutil.which(c):
            return c
    return None


def cmake_args() -> list[str]:
    """Builds the base cmake configure command."""
    args = ["cmake", "-S", str(SCRIPT_DIR), "-B", str(BUILD_DIR),
            "-DCMAKE_BUILD_TYPE=Release"]
    if cxx := pick_compiler():
        args.append(f"-DCMAKE_CXX_COMPILER={cxx}")
        log(f"Compiler: {cxx}")
    else:
        log("Compiler: auto-detected by CMake")
    return args


def jobs() -> list[str]:
    """Returns -j flag with CPU count."""
    count = os.cpu_count() or 4
    return ["-j", str(count)]


def do_build() -> None:
    """Configures and builds the project."""
    run(cmake_args())
    run(["cmake", "--build", str(BUILD_DIR)] + jobs())
    built = BUILD_DIR / BINARY_NAME
    if built.exists():
        shutil.copy2(built, BINARY_PATH)
        log(f"Built: {BINARY_PATH}")
    else:
        # Windows multi-config: check Release subdirectory
        built_release = BUILD_DIR / "Release" / BINARY_NAME
        if built_release.exists():
            shutil.copy2(built_release, BINARY_PATH)
            log(f"Built: {BINARY_PATH}")
        else:
            die(f"binary not found at {built} or {built_release}")


def do_test() -> None:
    """Builds and runs unit tests via ctest."""
    do_build()
    run(["ctest", "--test-dir", str(BUILD_DIR), "--output-on-failure"])


def do_clean() -> None:
    """Removes the build directory."""
    if BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)
    log("build/ removed")


def install_prefix() -> Path:
    """Determines the install prefix based on platform and privileges."""
    # User override via PREFIX environment variable
    if prefix := os.environ.get("PREFIX"):
        return Path(prefix)

    system = platform.system()

    if system == "Windows":
        # Windows convention: %LOCALAPPDATA%\Programs\mdsd
        local_app_data = os.environ.get("LOCALAPPDATA", str(Path.home() / "AppData" / "Local"))
        return Path(local_app_data) / "Programs"

    # Linux/macOS: /usr/local for root, ~/.local for regular users
    if os.geteuid() == 0 if hasattr(os, "geteuid") else False:
        return Path("/usr/local")
    return Path.home() / ".local"


def do_install() -> None:
    """Builds and installs to the platform-appropriate prefix."""
    do_build()
    prefix = install_prefix()
    run(["cmake", "--install", str(BUILD_DIR), "--prefix", str(prefix)])

    bin_dir = prefix / "bin"
    log(f"Installed to {bin_dir}")

    # Check if the install directory is in PATH
    path_dirs = os.environ.get("PATH", "").split(os.pathsep)
    if str(bin_dir) not in path_dirs:
        system = platform.system()
        if system == "Windows":
            log(f"Note: add {bin_dir} to PATH:")
            log(f'  setx PATH "%PATH%;{bin_dir}"')
        else:
            log(f"Note: {bin_dir} is not in PATH; add:")
            log(f'  export PATH="{bin_dir}:$PATH"')


def do_uninstall() -> None:
    """Removes installed files using the CMake install manifest."""
    manifest = BUILD_DIR / "install_manifest.txt"
    if not manifest.exists():
        die(f"{manifest} not found — run 'python3 install.py install' first")

    removed = 0
    for line in manifest.read_text().splitlines():
        f = Path(line.strip())
        if f.exists():
            f.unlink()
            print(f"  removed: {f}")
            removed += 1
    log(f"Removed {removed} files")


def main() -> None:
    parser = argparse.ArgumentParser(
        prog="install.py",
        description="Build and install mdsd (cross-platform)")
    parser.add_argument("command", nargs="?", default="install",
                        choices=["build", "test", "install", "uninstall", "clean"],
                        help="command to run (default: install)")
    args = parser.parse_args()

    # Sanity check: cmake must be available
    if not shutil.which("cmake"):
        die("cmake not found in PATH. Install CMake 3.16+ from https://cmake.org")

    commands = {
        "build": do_build,
        "test": do_test,
        "install": do_install,
        "uninstall": do_uninstall,
        "clean": do_clean,
    }
    commands[args.command]()


if __name__ == "__main__":
    main()
