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

set "PROJECT_DIR=%GHIDRA_HOME%\projects"
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

call "%DEV_ROOT%\dev_env.bat" find_game "FINAL FANTASY FFX&FFX-2 HD Remaster" GAME_ROOT
if not defined GAME_ROOT (
    echo ERROR: "FINAL FANTASY FFX&FFX-2 HD Remaster" not found in any Steam library.
    echo   Detected: %STEAM_LIBS%
    pause
    exit /b 1
)
set "GAME_DIR=%GAME_ROOT%"

set "OUTPUT_DIR=%FFX_EVIDENCE_ROOT%\ghidra-output\iggy"

echo.
echo ==========================================
echo   Ghidra Analysis: iggy_w32.dll
echo ==========================================
echo.
echo This imports iggy_w32.dll (the Flash/SWF runtime),
echo runs RTTI + Parameter ID + Function ID analysis,
echo then decompiles all functions and exports class names.
echo.
echo Expected runtime: 5-15 minutes.
echo.
pause

:: Create output directory
if not exist "!OUTPUT_DIR!" mkdir "!OUTPUT_DIR!"

:: Check if project already exists
set "REP_DIR=%PROJECT_DIR%\iggy.rep"
if exist "!REP_DIR!" (
    echo Project 'iggy' already exists. Running analysis on existing import...
    echo.

    echo [1/2] Running advanced analysis...
    set "LOG1=!OUTPUT_DIR!\analysis_console.log"
    call "%GHIDRA_HOME%\support\analyzeHeadless.bat" "%PROJECT_DIR%" "iggy" ^
        -process "iggy_w32.dll" ^
        -analysisTimeoutPerFile 3600 ^
        -max-cpu 6 ^
        -analyzer "Windows x86 PE RTTI Analyzer" true ^
        -analyzer "Decompiler Parameter ID" true ^
        -analyzer "Function ID" true ^
        -analyzer "RTTI Analyzer" true > "!LOG1!" 2>&1
    echo [1/2] Analysis complete.
) else (
    echo Project 'iggy' does not exist. Importing iggy_w32.dll...
    echo.

    echo [1/2] Importing + full analysis...
    set "LOG1=!OUTPUT_DIR!\analysis_console.log"
    call "%GHIDRA_HOME%\support\analyzeHeadless.bat" "%PROJECT_DIR%" "iggy" ^
        -import "%GAME_DIR%\iggy_w32.dll" ^
        -analysisTimeoutPerFile 3600 ^
        -max-cpu 6 ^
        -analyzer "Windows x86 PE RTTI Analyzer" true ^
        -analyzer "Decompiler Parameter ID" true ^
        -analyzer "Function ID" true ^
        -analyzer "RTTI Analyzer" true > "!LOG1!" 2>&1
    echo [1/2] Import + analysis complete.
)

:: Step 2: Run decompile + RTTI export script
echo.
echo [2/2] Decompiling all functions + exporting RTTI...
set "LOG2=!OUTPUT_DIR!\decompile_console.log"

call "%GHIDRA_HOME%\support\analyzeHeadless.bat" "%PROJECT_DIR%" "iggy" ^
    -process "iggy_w32.dll" ^
    -noanalysis ^
    -scriptPath "%SCRIPT_DIR%" ^
    -postScript "decompile_iggy.java" ^
    -max-cpu 6 > "!LOG2!" 2>&1

:: Check completion
findstr /C:"Decompile Complete" "!LOG2!" >nul 2>&1
if !errorlevel!==0 (
    echo [OK] Decompile completed successfully.
) else (
    echo [WARN] Check log: !LOG2!
)

echo.
echo ==========================================
echo   iggy_w32.dll Analysis Complete
echo ==========================================
echo.
echo Output files:
echo   !OUTPUT_DIR!\rtti_classes.txt    (class names, vtables)
echo   !OUTPUT_DIR!\decompile_all.txt   (full pseudocode)
echo.
pause
exit /b 0
