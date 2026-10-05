#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
"$project_dir/run.sh" --build-only
build_dir="${BUILD_DIR:-$project_dir/build}"
emulator="$build_dir/shell_emulator"
if [[ "$(uname -s)" == Darwin ]]; then
    emulator="$build_dir/shell_emulator.app/Contents/MacOS/shell_emulator"
fi

echo "Успешный скрипт завершит приложение командой exit."
"$emulator" --vfs demo.csv --script "$project_dir/examples/startup.txt"

echo "Скрипт остановится в строке 2. Проверьте ручной ввод, затем введите exit."
"$emulator" --vfs demo.csv --script "$project_dir/examples/startup-error.txt"

echo "Ошибка открытия скрипта. После проверки введите exit."
"$emulator" --vfs demo.csv --script "$project_dir/examples/missing.txt"
