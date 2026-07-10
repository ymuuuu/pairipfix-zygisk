#!/usr/bin/env bash
# Convenience wrapper around build.py for this dev machine.
# Usage:
#   ./build.sh                 # default: release zip (all supported ABIs)
#   ./build.sh -t debug zip    # any build.py args pass through
#   ./build.sh build -a arm64-v8a
# Override machine paths via env: NDK_DIR, NDK_VER, NINJA_DIR, FAKE_SDK.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"

: "${NDK_DIR:=/var/android-ndk-r27d}"       # standalone NDK location
: "${NDK_VER:=r27d}"                          # name exposed under <sdk>/ndk/
: "${NINJA_DIR:=/home/ymuu/miniconda3/bin}"   # dir containing the ninja binary
: "${FAKE_SDK:=$HERE/.build-sdk}"             # SDK layout build.py expects

# build.py resolves the NDK from <ANDROID_HOME>/ndk/<ndkVer>; bridge the standalone
# NDK into that layout with a symlink.
mkdir -p "$FAKE_SDK/ndk"
ln -sfn "$NDK_DIR" "$FAKE_SDK/ndk/$NDK_VER"
export ANDROID_HOME="$FAKE_SDK"
export PATH="$NINJA_DIR:$PATH"
# Host CMake >= 4.0 dropped compatibility with the vendored submodules' old
# cmake_minimum_required; this env var re-enables it. Harmless on older CMake.
export CMAKE_POLICY_VERSION_MINIMUM=3.5

# Default to a full release zip when no args are given.
if [ "$#" -eq 0 ]; then
    set -- -t release zip
fi

echo ">> python3 build.py --ndk $NDK_VER $*"
exec python3 "$HERE/build.py" --ndk "$NDK_VER" "$@"
