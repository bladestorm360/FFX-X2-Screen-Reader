@echo off
setlocal
:: The Steam game location comes from dev_env.bat at the Dev tree root, so this
:: file holds no drive letters and the tree can move to any drive.
call "%~dp0..\..\..\..\..\dev_env.bat"
if not defined DEV_ROOT (
    echo ERROR: dev_env.bat not found at %~dp0..\..\..\..\..\dev_env.bat
    exit /b 1
)
call "%DEV_ROOT%\dev_env.bat" find_game "FINAL FANTASY FFX&FFX-2 HD Remaster" GAME_ROOT
if not defined GAME_ROOT (
    echo ERROR: "FINAL FANTASY FFX&FFX-2 HD Remaster" not found in any Steam library.
    echo   Detected: %STEAM_LIBS%
    exit /b 1
)
set "GAME_DIR=%GAME_ROOT%"
set REPO_ROOT=%~dp0..\..

echo Deploying to: "%GAME_DIR%"
echo.

:: Validate actual compiled signatures, all dependencies, launcher bootstrap and
:: closed processes before backups or any live file replacement.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0verify_deployment.ps1" -GameDirectory "%GAME_DIR%" -RepositoryRoot "%REPO_ROOT%" -TolkRoot "%TOLK_ROOT%"
if errorlevel 1 exit /b 1

:: Validate the coordinated migration artifacts before changing live files.
for %%F in ("%~dp0dinput8.dll" "%REPO_ROOT%\install\native\SDL3.dll" "%REPO_ROOT%\install\native\SDL3_LICENSE.txt" "%REPO_ROOT%\install\native\MinHook.x86.dll" "%REPO_ROOT%\install\native\FFXLauncherReader.dll" "%REPO_ROOT%\install\native\FFXLauncherNative.dll") do (
    if not exist %%F (
        echo ERROR: Required deployment artifact missing: %%F
        exit /b 1
    )
)
if exist "%GAME_DIR%\dinput8.dll" if not exist "%GAME_DIR%\dinput8.dll.ffx_sr_before_native" (
    copy /Y "%GAME_DIR%\dinput8.dll" "%GAME_DIR%\dinput8.dll.ffx_sr_before_native"
    if errorlevel 1 exit /b 1
)

:: Proxy DLL
if exist "%GAME_DIR%\dinput8.dll" if not exist "%GAME_DIR%\dinput8.dll.ffx_sr_before_sdl" (
    copy /Y "%GAME_DIR%\dinput8.dll" "%GAME_DIR%\dinput8.dll.ffx_sr_before_sdl" >nul
    if errorlevel 1 exit /b 1
)
copy /Y "%~dp0dinput8.dll" "%GAME_DIR%\dinput8.dll"
if errorlevel 1 exit /b 1

:: x86 native hook and speech dependencies built with the proxy.
copy /Y "%REPO_ROOT%\install\native\SDL3.dll" "%GAME_DIR%\SDL3.dll" >nul
if errorlevel 1 exit /b 1
copy /Y "%REPO_ROOT%\install\native\SDL3_LICENSE.txt" "%GAME_DIR%\FFX_SR_SDL3_LICENSE.txt" >nul
if errorlevel 1 exit /b 1
if exist "%REPO_ROOT%\install\native\MinHook.x86.dll" (
    copy /Y "%REPO_ROOT%\install\native\MinHook.x86.dll" "%GAME_DIR%\MinHook.x86.dll"
    if errorlevel 1 exit /b 1
)

if errorlevel 1 exit /b 1
copy /Y "%REPO_ROOT%\src\native\DEPENDENCIES.md" "%GAME_DIR%\FFX_SR_Native_Dependencies.md" >nul
if errorlevel 1 exit /b 1

:: No Gadget or JavaScript is loaded or deployed. Existing legacy files are
:: retained solely for restore_before_native.bat's original proxy rollback.

if exist "%REPO_ROOT%\install\native\FFXLauncherReader.dll" (
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%REPO_ROOT%\src\launcher\install.ps1" -GameDirectory "%GAME_DIR%" -StagingDirectory "%REPO_ROOT%\install\native"
    if errorlevel 1 exit /b 1
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0verify_deployment.ps1" -GameDirectory "%GAME_DIR%" -RepositoryRoot "%REPO_ROOT%" -TolkRoot "%TOLK_ROOT%" -Phase Installed
if errorlevel 1 exit /b 1

echo.
echo Deploy complete.
endlocal
