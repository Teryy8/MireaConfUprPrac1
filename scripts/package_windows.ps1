# Собирает папку приложения с Qt, библиотеками MinGW, VFS и примерами в ZIP.
param(
    [Parameter(Mandatory)][string]$QtRoot,
    [Parameter(Mandatory)][string]$CompilerBin,
    [string]$BuildDir = 'build-windows',
    [string]$OutputDir = 'dist'
)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$output = Join-Path $project $OutputDir
$app = Join-Path $output 'ShellEmulator'
if (Test-Path $app) { Remove-Item $app -Recurse -Force }
New-Item $app -ItemType Directory -Force | Out-Null
Copy-Item (Join-Path $project "$BuildDir/shell_emulator.exe") $app
$env:PATH = "$CompilerBin;$QtRoot/bin;$env:PATH"
& "$QtRoot/bin/windeployqt.exe" --release --compiler-runtime --no-translations --dir $app "$app/shell_emulator.exe"
if ($LASTEXITCODE -ne 0) { throw 'windeployqt failed' }
# Явно добавляем библиотеки компилятора, чтобы запуск не зависел от PATH компьютера.
foreach ($dll in 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll') {
    Copy-Item (Join-Path $CompilerBin $dll) $app
}
foreach ($folder in 'vfs', 'examples') { Copy-Item (Join-Path $project $folder) $app -Recurse }
Copy-Item (Join-Path $project 'start.bat') $app
Copy-Item (Join-Path $project 'README.md') $app
@'
Эмулятор оболочки — вариант 17
Windows 10 (1809 и новее) / Windows 11, 64 бит.

1. Полностью распакуйте архив в отдельную папку.
2. Запустите start.bat. Устанавливать Qt или компилятор не нужно.
3. Вводите команды в окне приложения и нажимайте Enter.

По умолчанию загружается vfs/commands.csv: 7 каталогов, 12 файлов.
Примеры: ls /docs, tail -n 2 /docs/long.txt, history, exit.
Все команды и проверки описаны в README.md.
Для демонстрации из командной строки: start.bat --script examples/stage4.txt

Сохраняйте вместе exe, DLL, platforms и остальные папки из архива.
vfs-init очищает выбранный CSV на диске; проверяйте её на копии CSV.

Исходный код: https://github.com/Teryy8/MireaConfUprPrac1
Qt 6.8.3 используется как динамические библиотеки. Тексты лицензий — licenses.
Исходный код Qt: https://code.qt.io/cgit/qt/qtbase.git/tag/?h=v6.8.3
Библиотеки MinGW 13.1: https://www.mingw-w64.org/ и https://gcc.gnu.org/
'@ | Set-Content (Join-Path $app 'README.txt') -Encoding utf8
$licenses = Join-Path $app 'licenses'
New-Item $licenses -ItemType Directory -Force | Out-Null
foreach ($name in 'LGPL-3.0-only', 'GPL-3.0-only') {
    Invoke-WebRequest "https://raw.githubusercontent.com/qt/qtbase/v6.8.3/LICENSES/$name.txt" -OutFile "$licenses/$name.txt"
}
Invoke-WebRequest 'https://raw.githubusercontent.com/gcc-mirror/gcc/releases/gcc-13.1.0/COPYING.RUNTIME' -OutFile "$licenses/GCC-Runtime-Exception.txt"
Invoke-WebRequest 'https://raw.githubusercontent.com/mingw-w64/mingw-w64/v11.0.1/COPYING' -OutFile "$licenses/MinGW-w64-COPYING.txt"
$archive = Join-Path $output 'ShellEmulator-Windows-x64.zip'
if (Test-Path $archive) { Remove-Item $archive }
Compress-Archive -Path $app -DestinationPath $archive
Write-Host "Archive: $archive"
