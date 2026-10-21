@echo off
setlocal
call "%~dp0..\..\..\..\..\dev_env.bat"
if not defined DEV_ROOT exit /b 1
call "%DEV_ROOT%\dev_env.bat" find_game "FINAL FANTASY FFX&FFX-2 HD Remaster" GAME_ROOT
if not defined GAME_ROOT exit /b 1
if not exist "%GAME_ROOT%\dinput8.dll.ffx_sr_before_native" (
    echo ERROR: No proxy backup from native deployment exists.
    exit /b 1
)
copy /Y "%GAME_ROOT%\dinput8.dll.ffx_sr_before_native" "%GAME_ROOT%\dinput8.dll"
if errorlevel 1 exit /b 1
if exist "%GAME_ROOT%\FFX&X-2_LAUNCHER.exe.config.ffx_sr_original" (
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\launcher\install.ps1" -GameDirectory "%GAME_ROOT%" -StagingDirectory "%~dp0..\..\install\native" -Restore
    if errorlevel 1 exit /b 1
)
echo Original proxy restored. Its installed legacy Gadget/bundle files were retained.
echo Offline prototype builds are not deployed by this restore.
echo Close and restart the game and launcher to use the restored proxy/configuration.
exit /b 0
