@echo off
setlocal enabledelayedexpansion

call "%~dp0..\evidence_root.bat"
if errorlevel 1 exit /b 1

:: ============================================
:: FFX/FFX-2 HD Remaster - Ghidra Import All
:: One-time script: imports all 4 executables
:: and runs full analysis on each.
:: ============================================

:: Ghidra and the Steam game location come from dev_env.bat at the Dev tree root,
:: so this file holds no drive letters and the tree can move to any drive.
call "%~dp0..\..\..\..\..\..\dev_env.bat"
if not defined DEV_ROOT (
    echo ERROR: dev_env.bat not found at %~dp0..\..\..\..\..\..\dev_env.bat
    pause
    exit /b 1
)

set "PROJECT_DIR=%GHIDRA_HOME%\projects"

call "%DEV_ROOT%\dev_env.bat" find_game "FINAL FANTASY FFX&FFX-2 HD Remaster" GAME_ROOT
if not defined GAME_ROOT (
    echo ERROR: "FINAL FANTASY FFX&FFX-2 HD Remaster" not found in any Steam library.
    echo   Detected: %STEAM_LIBS%
    pause
    exit /b 1
)
set "GAME_DIR=%GAME_ROOT%"

set "OUTPUT_DIR=%FFX_EVIDENCE_ROOT%\ghidra-output"

if not exist "%PROJECT_DIR%" mkdir "%PROJECT_DIR%"

:: Define all executables and their project names
set "EXE_1=FFX.exe"
set "PROJ_1=FFX"
set "EXE_2=FFX-2.exe"
set "PROJ_2=FFX2"
set "EXE_3=FFX&X-2_Will.exe"
set "PROJ_3=FFX_Will"
set "EXE_4=FFX&X-2_LAUNCHER.exe"
set "PROJ_4=FFX_Launcher"
set "TOTAL=4"

echo.
echo ============================================
echo  FFX/FFX-2 HD Remaster - Import All
echo  Importing and analyzing %TOTAL% executables
echo ============================================
echo.

set "PASS=0"
set "SKIP=0"
set "FAIL=0"

for /L %%i in (1,1,%TOTAL%) do call :import_exe %%i

echo.
echo ============================================
echo  IMPORT SUMMARY
echo  Passed:  !PASS!
echo  Skipped: !SKIP! (already imported)
echo  Failed:  !FAIL!
echo  Total:   %TOTAL%
echo ============================================
echo.
if !FAIL! GTR 0 (
    echo Some imports failed. Check logs in %OUTPUT_DIR%
)
pause
exit /b 0

:: ---- Subroutine: import and analyze one executable ----
:import_exe
set "IDX=%1"
set "EXE=!EXE_%IDX%!"
set "PROJ=!PROJ_%IDX%!"
set "EXE_PATH=%GAME_DIR%\!EXE!"
set "PROJ_OUTPUT_DIR=%OUTPUT_DIR%\!PROJ!"

if not exist "!PROJ_OUTPUT_DIR!" mkdir "!PROJ_OUTPUT_DIR!"

echo.
echo [%IDX%/%TOTAL%] !EXE! -> Project: !PROJ!

:: Check if executable exists
if not exist "!EXE_PATH!" (
    echo [FAIL] %IDX%/%TOTAL%. !EXE! -- executable not found at !EXE_PATH!
    set /a FAIL+=1
    exit /b
)

:: Check if project already exists
set "REP_DIR=%PROJECT_DIR%\!PROJ!.rep"
if exist "!REP_DIR!" (
    echo [SKIP] %IDX%/%TOTAL%. !PROJ! -- project already exists
    set /a SKIP+=1
    exit /b
)

set "IMPORT_LOG=!PROJ_OUTPUT_DIR!\import_analysis_console.log"

echo [%date% %time%] Starting import + full analysis...
echo   Log: !IMPORT_LOG!

call "%GHIDRA_HOME%\support\analyzeHeadless.bat" "%PROJECT_DIR%" "!PROJ!" ^
    -import "!EXE_PATH!" ^
    -analysisTimeoutPerFile 3600 ^
    -max-cpu 6 > "!IMPORT_LOG!" 2>&1

if !errorlevel! equ 0 (
    echo [OK]   %IDX%/%TOTAL%. !PROJ! -- import + analysis complete
    set /a PASS+=1
) else (
    echo [FAIL] %IDX%/%TOTAL%. !PROJ! -- check !IMPORT_LOG!
    set /a FAIL+=1
)

echo [%date% %time%] Finished !PROJ!.
exit /b
