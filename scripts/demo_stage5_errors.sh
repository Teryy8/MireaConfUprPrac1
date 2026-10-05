#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
"$project_dir/run.sh" --build-only
build_dir="${BUILD_DIR:-$project_dir/build}"
emulator="$build_dir/shell_emulator"
if [[ "$(uname -s)" == Darwin ]]; then
    emulator="$build_dir/shell_emulator.app/Contents/MacOS/shell_emulator"
fi

for script in "$project_dir/examples/stage5-errors/"*.txt; do
    echo "Проверка $(basename -- "$script"): после остановки скрипта введите exit."
    "$emulator" --vfs "$project_dir/vfs/commands.csv" --script "$script"
done
