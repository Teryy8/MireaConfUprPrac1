#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
"$project_dir/run.sh" --build-only
build_dir="${BUILD_DIR:-$project_dir/build}"
emulator="$build_dir/shell_emulator"
if [[ "$(uname -s)" == Darwin ]]; then
    emulator="$build_dir/shell_emulator.app/Contents/MacOS/shell_emulator"
fi
vfs_test_dir="$(mktemp -d)"
trap 'rm -rf -- "$vfs_test_dir"' EXIT

for name in minimal files nested; do
    cp "$project_dir/vfs/$name.csv" "$vfs_test_dir/$name.csv"
    echo "После ошибки vfs-init extra проверьте ручной ввод и введите exit."
    "$emulator" --vfs "$vfs_test_dir/$name.csv" --script "$project_dir/examples/stage3.txt"
    if [[ -s "$vfs_test_dir/$name.csv" ]]; then
        echo "Ошибка: CSV не был очищен." >&2
        exit 1
    fi
    "$emulator" --vfs "$vfs_test_dir/$name.csv" --script "$project_dir/examples/exit.txt"
done
