@echo off
setlocal enabledelayedexpansion

call "%~dp0..\evidence_root.bat"
if errorlevel 1 exit /b 1

:: Ghidra comes from dev_env.bat at the Dev tree root, so this file holds no
:: drive letters and the tree can move to any drive.
call "%~dp0..\..\..\..\..\..\dev_env.bat"
if not defined DEV_ROOT (
    echo ERROR: dev_env.bat not found at %~dp0..\..\..\..\..\..\dev_env.bat
    pause
    exit /b 1
)

set "PROJECT_DIR=%GHIDRA_HOME%\projects"
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "OUTPUT_DIR=%FFX_EVIDENCE_ROOT%\ghidra-output"

echo.
echo ==========================================
echo   Advanced Ghidra Analysis - ALL Games
echo ==========================================
echo.
echo This runs RTTI, Decompiler Parameter ID,
echo and Function ID analyzers on all 4 executables,
echo then exports class names and function signatures.
echo.
echo Expected runtime: 5-30 minutes per executable.
echo.
pause

:: Process each game
:: Note: & in filenames must use delayed expansion to avoid double-caret issues with call
set "EXE_FFX=FFX.exe"
set "EXE_FFX2=FFX-2.exe"
set "EXE_WILL=FFX&X-2_Will.exe"
set "EXE_LAUNCHER=FFX&X-2_LAUNCHER.exe"
call :process_game "!EXE_FFX!" "FFX"
call :process_game "!EXE_FFX2!" "FFX2"
call :process_game "!EXE_WILL!" "FFX_Will"
call :process_game "!EXE_LAUNCHER!" "FFX_Launcher"

echo.
echo ==========================================
echo   ALL GAMES COMPLETE
echo ==========================================
echo.
echo Output files:
echo   %OUTPUT_DIR%\FFX\rtti_classes.txt
echo   %OUTPUT_DIR%\FFX2\rtti_classes.txt
echo   %OUTPUT_DIR%\FFX_Will\rtti_classes.txt
echo   %OUTPUT_DIR%\FFX_Launcher\rtti_classes.txt
echo.
pause
exit /b 0

:: ========================================
:: Subroutine: process one game
:: %1 = exe name, %2 = project name
:: ========================================
:process_game
set "GAME_EXE=%~1"
set "PROJECT_NAME=%~2"
set "GAME_OUTPUT_DIR=%OUTPUT_DIR%\%PROJECT_NAME%"

echo.
echo ============================================
echo  Processing: !GAME_EXE! (Project: !PROJECT_NAME!)
echo ============================================

:: Check project exists
set "REP_DIR=%PROJECT_DIR%\%PROJECT_NAME%.rep"
if not exist "!REP_DIR!" (
    echo [SKIP] Project %PROJECT_NAME% not found. Run full_analysis first.
    echo.
    goto :eof
)

:: Create output dir
if not exist "!GAME_OUTPUT_DIR!" mkdir "!GAME_OUTPUT_DIR!"

:: Step 1: Run advanced analyzers
echo.
echo [1/2] Running RTTI + Parameter ID + Function ID analysis...
set "ANALYSIS_LOG=!GAME_OUTPUT_DIR!\advanced_analysis_console.log"

call "%GHIDRA_HOME%\support\analyzeHeadless.bat" "%PROJECT_DIR%" "%PROJECT_NAME%" ^
    -process "!GAME_EXE!" ^
    -analysisTimeoutPerFile 3600 ^
    -max-cpu 6 ^
    -analyzer "Windows x86 PE RTTI Analyzer" true ^
    -analyzer "Decompiler Parameter ID" true ^
    -analyzer "Function ID" true ^
    -analyzer "RTTI Analyzer" true > "!ANALYSIS_LOG!" 2>&1

echo [1/2] Analysis complete. Checking results...
findstr /C:"ERROR" /C:"error" "!ANALYSIS_LOG!" 2>nul | findstr /V /C:"error code" >nul 2>&1
if !errorlevel!==0 (
    echo [WARN] Errors found in analysis log. Check: !ANALYSIS_LOG!
) else (
    echo [OK] Analysis completed without errors.
)

:: Step 2: Run export script
echo.
echo [2/2] Exporting RTTI classes and function signatures...
set "EXPORT_LOG=!GAME_OUTPUT_DIR!\export_rtti_console.log"

call "%GHIDRA_HOME%\support\analyzeHeadless.bat" "%PROJECT_DIR%" "%PROJECT_NAME%" ^
    -process "!GAME_EXE!" ^
    -noanalysis ^
    -scriptPath "%SCRIPT_DIR%" ^
    -postScript "export_rtti.java" ^
    -max-cpu 6 > "!EXPORT_LOG!" 2>&1

:: Check for completion
findstr /C:"Export Complete" "!EXPORT_LOG!" >nul 2>&1
if !errorlevel!==0 (
    echo [OK] Export completed. Output: !GAME_OUTPUT_DIR!\rtti_classes.txt
) else (
    echo [WARN] Export may have issues. Check: !EXPORT_LOG!
)

echo.
echo  %PROJECT_NAME% done.
echo ============================================
goto :eof
