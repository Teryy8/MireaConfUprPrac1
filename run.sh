#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${BUILD_DIR:-$project_dir/build}"

cmake_args=(-S "$project_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Debug)
if [[ -n "${QT_ROOT:-}" ]]; then
    cmake_args+=("-DCMAKE_PREFIX_PATH=$QT_ROOT")
elif [[ -z "${CMAKE_PREFIX_PATH:-}" ]] && ! command -v qt-cmake >/dev/null 2>&1; then
    shopt -s nullglob
    for qt_config in "$HOME"/Qt/*/macos/lib/cmake/Qt6/Qt6Config.cmake; do
        qt_root="${qt_config%/lib/cmake/Qt6/Qt6Config.cmake}"
        break
    done
    if [[ -n "${qt_root:-}" ]]; then
        cmake_args+=("-DCMAKE_PREFIX_PATH=$qt_root")
    fi
fi

if command -v qt-cmake >/dev/null 2>&1 && [[ -z "${QT_ROOT:-}" ]]; then
    qt-cmake "${cmake_args[@]}"
else
    cmake "${cmake_args[@]}"
fi
cmake --build "$build_dir" --parallel

case "${1:-}" in
    --test) exec ctest --test-dir "$build_dir" --output-on-failure ;;
    --build-only) exit 0 ;;
    "") ;;
    *) echo "Использование: ./run.sh [--test | --build-only]" >&2; exit 2 ;;
esac

if [[ "$(uname -s)" == Darwin ]]; then
    exec "$build_dir/shell_emulator.app/Contents/MacOS/shell_emulator"
fi
exec "$build_dir/shell_emulator"
