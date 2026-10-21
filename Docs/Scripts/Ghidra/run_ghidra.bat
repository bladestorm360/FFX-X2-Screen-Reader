@echo off
setlocal enabledelayedexpansion

call "%~dp0..\evidence_root.bat"
if errorlevel 1 exit /b 1

:: Ghidra and the Steam game location come from dev_env.bat at the Dev tree root,
:: so this file holds no drive letters and the tree can move to any drive.
call "%~dp0..\..\..\..\..\..\dev_env.bat"
if not defined DEV_ROOT (
    echo ERROR: dev_env.bat not found at %~dp0..\..\..\..\..\..\dev_env.bat
    pause
    exit /b 1
)

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "OUTPUT_DIR=%FFX_EVIDENCE_ROOT%\ghidra-output"
set "PROJECT_DIR=%GHIDRA_HOME%\projects"

:: Game installation directory
call "%DEV_ROOT%\dev_env.bat" find_game "FINAL FANTASY FFX&FFX-2 HD Remaster" GAME_ROOT
if not defined GAME_ROOT (
    echo ERROR: "FINAL FANTASY FFX&FFX-2 HD Remaster" not found in any Steam library.
    echo   Detected: %STEAM_LIBS%
    pause
    exit /b 1
)
set "GAME_DIR=%GAME_ROOT%"

echo.
echo ==========================================
echo   Ghidra Headless Analysis - FFX/FFX-2
echo ==========================================
echo.
echo Select game executable:
echo   1. FFX.exe              (Final Fantasy X)
echo   2. FFX-2.exe            (Final Fantasy X-2)
echo   3. FFX^&X-2_Will.exe     (FFX-2 Last Mission)
echo   4. FFX^&X-2_LAUNCHER.exe (Game Selector)
echo.
set /p "GAME_CHOICE=Select game (1-4): "

if "!GAME_CHOICE!"=="1" (
    set "GAME_EXE=FFX.exe"
    set "PROJECT_NAME=FFX"
    set "GAME_SCRIPT_DIR=%SCRIPT_DIR%\FFX"
    set "GAME_OUTPUT_DIR=%OUTPUT_DIR%\FFX"
)
if "!GAME_CHOICE!"=="2" (
    set "GAME_EXE=FFX-2.exe"
    set "PROJECT_NAME=FFX2"
    set "GAME_SCRIPT_DIR=%SCRIPT_DIR%\FFX2"
    set "GAME_OUTPUT_DIR=%OUTPUT_DIR%\FFX2"
)
if "!GAME_CHOICE!"=="3" (
    set "GAME_EXE=FFX&X-2_Will.exe"
    set "PROJECT_NAME=FFX_Will"
    set "GAME_SCRIPT_DIR=%SCRIPT_DIR%\FFX_Will"
    set "GAME_OUTPUT_DIR=%OUTPUT_DIR%\FFX_Will"
)
if "!GAME_CHOICE!"=="4" (
    set "GAME_EXE=FFX&X-2_LAUNCHER.exe"
    set "PROJECT_NAME=FFX_Launcher"
    set "GAME_SCRIPT_DIR=%SCRIPT_DIR%\FFX_Launcher"
    set "GAME_OUTPUT_DIR=%OUTPUT_DIR%\FFX_Launcher"
)

if not defined PROJECT_NAME (
    echo Invalid selection.
    pause
    exit /b 1
)

set "GAME_EXE_PATH=%GAME_DIR%\%GAME_EXE%"

if not exist "%GAME_EXE_PATH%" (
    echo ERROR: Game executable not found at:
    echo   %GAME_EXE_PATH%
    pause
    exit /b 1
)
echo Found: %GAME_EXE% (Project: %PROJECT_NAME%)

:: Create output directory
if not exist "!GAME_OUTPUT_DIR!" mkdir "!GAME_OUTPUT_DIR!"

:: Create project directory
if not exist "%PROJECT_DIR%" mkdir "%PROJECT_DIR%"

:: Select operation
echo.
echo Available operations:
echo   1. full_analysis    - Import (if needed) + full analysis + decompile
echo   2. project_status   - Quick diagnostic: what analysis is already done?
echo   3. run_script       - Run a .java script from the game's script folder
echo.
set /p "SCRIPT_NAME=Select operation (1-3): "

if "!SCRIPT_NAME!"=="1" set "SCRIPT_NAME=full_analysis"
if "!SCRIPT_NAME!"=="2" set "SCRIPT_NAME=project_status"
if "!SCRIPT_NAME!"=="3" set "SCRIPT_NAME=run_script"

echo.
echo ============================================
echo  Ghidra Headless Analysis
echo  Game: %GAME_EXE%
echo  Project: %PROJECT_NAME%
echo  Operation: !SCRIPT_NAME!
echo ============================================
echo.

if "!SCRIPT_NAME!"=="full_analysis" goto :do_full_analysis
if "!SCRIPT_NAME!"=="project_status" goto :do_project_status
if "!SCRIPT_NAME!"=="run_script" goto :do_run_script

echo ERROR: No handler for "!SCRIPT_NAME!".
echo Valid options: 1-3
pause
exit /b 1

:: ---- Handler: full_analysis (option 1) ----
:: Imports the executable (if not already in project) and runs full analysis
:: including decompiler. For 10-11MB binaries this should complete in minutes.
:do_full_analysis
echo ============================================
echo  FULL IMPORT + ANALYSIS
echo  Importing and analyzing %GAME_EXE%
echo  with all analyzers including decompiler.
echo  Expected runtime: minutes for 10-11MB exe.
echo ============================================
echo.

set "REP_DIR=%PROJECT_DIR%\%PROJECT_NAME%.rep"
set "ANALYSIS_LOG=!GAME_OUTPUT_DIR!\full_analysis_console.log"

:: Check if project already exists
if exist "!REP_DIR!" (
    echo Project %PROJECT_NAME% already exists. Running analysis on existing import...
    echo.

    echo [%date% %time%] Starting analysis on existing project...
    echo   Log: !ANALYSIS_LOG!
    echo.
    call "%GHIDRA_HOME%\support\analyzeHeadless.bat" "%PROJECT_DIR%" "%PROJECT_NAME%" ^
        -process "%GAME_EXE%" ^
        -analysisTimeoutPerFile 3600 ^
        -max-cpu 6 > "!ANALYSIS_LOG!" 2>&1
) else (
    echo Project %PROJECT_NAME% does not exist. Importing %GAME_EXE%...
    echo.

    echo [%date% %time%] Starting import + analysis...
    echo   Log: !ANALYSIS_LOG!
    echo.
    call "%GHIDRA_HOME%\support\analyzeHeadless.bat" "%PROJECT_DIR%" "%PROJECT_NAME%" ^
        -import "%GAME_EXE_PATH%" ^
        -analysisTimeoutPerFile 3600 ^
        -max-cpu 6 > "!ANALYSIS_LOG!" 2>&1
)

echo.
echo [%date% %time%] Analysis exited.
echo.
echo === ANALYSIS SUMMARY ===
findstr /C:"ANALYZING" "!ANALYSIS_LOG!" 2>nul
findstr /C:"ANALYSIS TIME" "!ANALYSIS_LOG!" 2>nul
findstr /C:"REPORT:" "!ANALYSIS_LOG!" 2>nul
findstr /C:"Functions:" "!ANALYSIS_LOG!" 2>nul
findstr /C:"Instructions:" "!ANALYSIS_LOG!" 2>nul
echo ========================
echo.
goto :done

:: ---- Handler: project_status (option 2) ----
:: Read-only diagnostic: reports what analysis is already done.
:do_project_status
set "REP_DIR=%PROJECT_DIR%\%PROJECT_NAME%.rep"
if not exist "!REP_DIR!" (
    echo ERROR: Project %PROJECT_NAME% not found. Run full_analysis first.
    pause
    exit /b 1
)

echo Running project_status on %PROJECT_NAME% (read-only)...
echo.
set "ANALYSIS_LOG=!GAME_OUTPUT_DIR!\project_status_console.log"

:: Check if we have a project_status script, otherwise just process with no analysis
if exist "!GAME_SCRIPT_DIR!\project_status.java" (
    call "%GHIDRA_HOME%\support\analyzeHeadless.bat" "%PROJECT_DIR%" "%PROJECT_NAME%" ^
        -process "%GAME_EXE%" ^
        -noanalysis ^
        -scriptPath "!GAME_SCRIPT_DIR!" ^
        -postScript "project_status.java" ^
        -max-cpu 6 > "!ANALYSIS_LOG!" 2>&1
) else (
    echo No project_status.java found in !GAME_SCRIPT_DIR!
    echo Running basic headless process to verify project loads...
    call "%GHIDRA_HOME%\support\analyzeHeadless.bat" "%PROJECT_DIR%" "%PROJECT_NAME%" ^
        -process "%GAME_EXE%" ^
        -noanalysis ^
        -max-cpu 6 > "!ANALYSIS_LOG!" 2>&1
)

echo.
echo === PROJECT STATUS ===
findstr /C:"FUNCTIONS:" /C:"Total:" /C:"PROGRAM INFO" /C:"Name:" /C:"Language:" /C:"Functions:" /C:"Instructions:" /C:"Defined" "!ANALYSIS_LOG!" 2>nul
echo =======================
goto :done

:: ---- Handler: run_script (option 3) ----
:: Dynamic discovery: lists .java files from the game's script directory.
:do_run_script
set "REP_DIR=%PROJECT_DIR%\%PROJECT_NAME%.rep"
if not exist "!REP_DIR!" (
    echo ERROR: Project %PROJECT_NAME% not found. Run full_analysis first.
    pause
    exit /b 1
)

echo.
echo Available scripts for %PROJECT_NAME%:
echo.

set SCOUNT=0

for %%F in ("!GAME_SCRIPT_DIR!\*.java") do (
    set /a SCOUNT+=1
    set "CS_!SCOUNT!=%%~nF"
    set "CS_PATH_!SCOUNT!=%%F"
    echo  !SCOUNT!. %%~nF
)

if !SCOUNT! equ 0 (
    echo No scripts found in !GAME_SCRIPT_DIR!
    pause
    exit /b 1
)

echo.
set /p "CS_CHOICE=Select script: "

for %%I in (!CS_CHOICE!) do set "CS_SCRIPT=!CS_%%I!"

if not defined CS_SCRIPT (
    echo Invalid selection.
    pause
    exit /b 1
)

set "CS_LOG=!GAME_OUTPUT_DIR!\!CS_SCRIPT!_console.log"
set "CS_SCRIPTLOG=!GAME_OUTPUT_DIR!\!CS_SCRIPT!_output.log"

echo.
echo Running: !CS_SCRIPT!.java
echo Mode:    -process -noanalysis
echo Output:  !CS_SCRIPTLOG!
echo.

call "%GHIDRA_HOME%\support\analyzeHeadless.bat" "%PROJECT_DIR%" "%PROJECT_NAME%" ^
    -process "%GAME_EXE%" ^
    -noanalysis ^
    -scriptPath "!GAME_SCRIPT_DIR!" ^
    -postScript "!CS_SCRIPT!.java" ^
    -scriptlog "!CS_SCRIPTLOG!" ^
    -max-cpu 6 > "!CS_LOG!" 2>&1

echo.
echo [%date% %time%] Script finished.

:: Check for completion
findstr /C:"Complete ===" "!CS_LOG!" >nul 2>&1
if !errorlevel!==0 (
    echo [OK] !CS_SCRIPT! completed successfully.
) else (
    echo [CHECK] !CS_SCRIPT! - verify output in !CS_LOG!
)
goto :done

:done
echo.
echo ============================================
echo  Complete. Output log:
echo  !ANALYSIS_LOG!
echo ============================================
pause
exit /b 0
