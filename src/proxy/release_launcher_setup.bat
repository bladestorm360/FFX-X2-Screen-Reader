@echo off
setlocal
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0InstallLauncher.ps1" %*
if errorlevel 1 (
    echo Screen reader installation failed. Read the error above.
    pause
    exit /b 1
)
pause
