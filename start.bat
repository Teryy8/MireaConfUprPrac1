@echo off
setlocal
cd /d "%~dp0"
"%~dp0shell_emulator.exe" --vfs "%~dp0vfs\commands.csv" %*
if errorlevel 1 pause
