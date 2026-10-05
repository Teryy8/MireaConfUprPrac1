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
    "$emulator" --vfs "$vfs_test_dir/$name.csv" --script "$project_dir/examples/stage3-load.txt"
    cmp "$project_dir/vfs/$name.csv" "$vfs_test_dir/$name.csv"
done

"$emulator" --vfs "$vfs_test_dir/missing.csv" --script "$project_dir/examples/exit.txt"
"$emulator" --vfs "$project_dir/vfs/invalid.csv" --script "$project_dir/examples/exit.txt"
echo "Проверка вариантов VFS завершена."
