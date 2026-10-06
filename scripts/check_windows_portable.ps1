# Проверяет запуск распакованного архива с системным PATH без установленного Qt.
param([string]$Archive = 'dist/ShellEmulator-Windows-x64.zip')
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$check = Join-Path $env:RUNNER_TEMP 'Portable check/Эмулятор'
Expand-Archive (Join-Path $project $Archive) $check -Force
$app = Join-Path $check 'ShellEmulator'
foreach ($file in 'shell_emulator.exe', 'Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Widgets.dll',
    'platforms/qwindows.dll', 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll',
    'start.bat', 'vfs/commands.csv') {
    if (!(Test-Path (Join-Path $app $file))) { throw "Missing file: $file" }
}
$oldPath = $env:PATH
try {
    $env:PATH = "$env:SystemRoot/System32;$env:SystemRoot"
    foreach ($name in 'QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH', 'QT_QPA_PLATFORM', 'QML2_IMPORT_PATH') {
        Remove-Item "Env:$name" -ErrorAction SilentlyContinue
    }
    $vfs = Join-Path $app 'vfs/commands.csv'
    $originalHash = (Get-FileHash $vfs).Hash
    Push-Location $env:RUNNER_TEMP
    try {
        foreach ($scenario in 'stage4', 'stage5') {
            $stdout = Join-Path $check "$scenario.out.txt"
            $stderr = Join-Path $check "$scenario.err.txt"
            $arguments = '--vfs "{0}" --script "{1}"' -f $vfs, (Join-Path $app "examples/$scenario.txt")
            $process = Start-Process "$app/shell_emulator.exe" -ArgumentList $arguments -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
            if (!$process.WaitForExit(30000)) { $process.Kill(); throw "$scenario did not finish" }
            $text = Get-Content $stdout -Raw -Encoding utf8
            $errors = Get-Content $stderr -Raw -Encoding utf8
            if ($process.ExitCode -ne 0 -or $text -match 'Ошибка' -or $errors -match 'failed|could not|ошибка') {
                Write-Host $text
                Write-Host $errors
                throw "$scenario failed: exit $($process.ExitCode)"
            }
            if ($text -notmatch 'Каталогов: 7, файлов: 12' -or $text -notmatch 'history') { throw "$scenario output incomplete" }
            Write-Host "$scenario passed without Qt in PATH"
        }
        if ((Get-FileHash $vfs).Hash -ne $originalHash) { throw 'Original VFS was changed' }
        # Отдельно проверяем пользовательский BAT из каталога с пробелами и кириллицей.
        $command = '""{0}" --script "{1}""' -f (Join-Path $app 'start.bat'), (Join-Path $app 'examples/exit.txt')
        $process = Start-Process "$env:SystemRoot/System32/cmd.exe" -ArgumentList @('/d', '/s', '/c', $command) -PassThru
        if (!$process.WaitForExit(20000)) { $process.Kill(); throw 'start.bat did not finish' }
        if ($process.ExitCode -ne 0) { throw 'start.bat failed' }
        Write-Host 'start.bat passed'
    } finally { Pop-Location }
} finally { $env:PATH = $oldPath }
