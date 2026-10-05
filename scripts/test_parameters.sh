#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"
"$project_dir/run.sh" --build-only
build_dir="${BUILD_DIR:-$project_dir/build}"
emulator="$build_dir/shell_emulator"
if [[ "$(uname -s)" == Darwin ]]; then
    emulator="$build_dir/shell_emulator.app/Contents/MacOS/shell_emulator"
fi

"$emulator" --help
"$emulator" --version
"$emulator" --script "$project_dir/examples/exit.txt"
"$emulator" --vfs "$project_dir/vfs/files.csv" --script "$project_dir/examples/startup.txt"

expect_error() {
    if "$emulator" "$@"; then
        echo "Ожидалась ошибка параметров: $*" >&2
        exit 1
    else
        status=$?
        if [[ "$status" -ne 2 ]]; then
            echo "Неожиданный код завершения: $status" >&2
            exit 1
        fi
    fi
}

expect_error --vfs
expect_error --script
expect_error --vfs "" --script "$project_dir/examples/exit.txt"
expect_error --vfs VFS-17.csv --script ""
expect_error --unknown
echo "Проверка параметров завершена."
