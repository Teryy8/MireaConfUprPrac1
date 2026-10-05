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
cp "$project_dir/vfs/commands.csv" "$vfs_test_dir/commands.csv"

output="$("$emulator" --vfs "$vfs_test_dir/commands.csv" --script "$project_dir/examples/stage4.txt")"
printf '%s\n' "$output"
if [[ "$output" == *"Ошибка"* || "$output" == *"Неверный формат"* ]]; then
    echo "Сценарий завершился с ошибкой." >&2
    exit 1
fi
cmp "$project_dir/vfs/commands.csv" "$vfs_test_dir/commands.csv"
echo "Проверка команд завершена; исходный CSV не изменён."
