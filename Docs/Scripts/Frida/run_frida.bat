@echo off
setlocal enabledelayedexpansion

call "%~dp0..\evidence_root.bat"
if errorlevel 1 exit /b 1

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "OUTPUT_DIR=%FFX_EVIDENCE_ROOT%\frida-output"

echo.
echo ==========================================
echo   Frida Script Launcher - FFX/FFX-2
echo ==========================================
echo.
echo Select game process:
echo   1. FFX.exe              (Final Fantasy X)
echo   2. FFX-2.exe            (Final Fantasy X-2)
echo   3. FFX^&X-2_Will.exe     (FFX-2 Last Mission)
echo   4. FFX^&X-2_LAUNCHER.exe (Game Selector)
echo.
set /p "GAME_CHOICE=Select game (1-4): "

if "!GAME_CHOICE!"=="1" (
    set "GAME_PROC=FFX.exe"
    set "GAME_SCRIPT_DIR=%SCRIPT_DIR%\FFX"
    set "GAME_OUTPUT_DIR=%OUTPUT_DIR%\FFX"
)
if "!GAME_CHOICE!"=="2" (
    set "GAME_PROC=FFX-2.exe"
    set "GAME_SCRIPT_DIR=%SCRIPT_DIR%\FFX2"
    set "GAME_OUTPUT_DIR=%OUTPUT_DIR%\FFX2"
)
if "!GAME_CHOICE!"=="3" (
    set "GAME_PROC=FFX&X-2_Will.exe"
    set "GAME_SCRIPT_DIR=%SCRIPT_DIR%\FFX_Will"
    set "GAME_OUTPUT_DIR=%OUTPUT_DIR%\FFX_Will"
)
if "!GAME_CHOICE!"=="4" (
    set "GAME_PROC=FFX&X-2_LAUNCHER.exe"
    set "GAME_SCRIPT_DIR=%SCRIPT_DIR%\FFX_Launcher"
    set "GAME_OUTPUT_DIR=%OUTPUT_DIR%\FFX_Launcher"
)

if not defined GAME_PROC (
    echo Invalid selection.
    pause
    exit /b 1
)

if not exist "!GAME_OUTPUT_DIR!" mkdir "!GAME_OUTPUT_DIR!"

echo.
echo Target process: !GAME_PROC!
echo.

:: Dynamic script discovery - lists all .js files in the game's script directory.
:: To add a new script: drop the .js file in the game folder and it appears in the menu.
:: To retire a script: move it to archive\ (hidden unless you type 'a').
:: Tags in script comments:
::   // @auto    = one-shot mode, runs with --no-auto-reload
::   // @python  = requires Python host, launches <name>_host.py instead of frida
::   // @desc    = description shown in menu, text after "// @desc "

set COUNT=0
for %%F in ("!GAME_SCRIPT_DIR!\*.js") do (
    set /a COUNT+=1
    set "SCRIPT_!COUNT!=%%~nF"
    set "SPATH_!COUNT!=%%F"
)

if !COUNT! equ 0 (
    echo No .js scripts found in !GAME_SCRIPT_DIR!
    pause
    exit /b 1
)

:: Extract mode and description from each script via subroutine
for /l %%I in (1,1,!COUNT!) do call :get_info %%I

:: Display menu
for /l %%I in (1,1,!COUNT!) do (
    set "_D=!DESC_%%I!"
    if defined _D (
        echo  %%I. [!MODE_%%I!] !SCRIPT_%%I! -- !_D!
    ) else (
        echo  %%I. [!MODE_%%I!] !SCRIPT_%%I!
    )
)

echo.
echo  Type 'a' to list archived scripts.
echo.
set /p "CHOICE=Select script: "

if /i "!CHOICE!"=="a" (
    echo.
    set "ACOUNT=0"
    set "FIRST_ARCHIVE=!COUNT!"
    set /a FIRST_ARCHIVE+=1
    for %%F in ("!GAME_SCRIPT_DIR!\archive\*.js") do (
        set /a COUNT+=1
        set /a ACOUNT+=1
        set "SCRIPT_!COUNT!=%%~nF"
        set "SPATH_!COUNT!=%%F"
    )
    if !ACOUNT! equ 0 (
        echo No archived scripts found.
        pause
        exit /b 1
    )
    for /l %%I in (!FIRST_ARCHIVE!,1,!COUNT!) do call :get_info %%I
    for /l %%I in (!FIRST_ARCHIVE!,1,!COUNT!) do (
        set "_D=!DESC_%%I!"
        if defined _D (
            echo  %%I. [!MODE_%%I!] !SCRIPT_%%I! -- !_D!
        ) else (
            echo  %%I. [!MODE_%%I!] !SCRIPT_%%I!
        )
    )
    echo.
    set /p "CHOICE=Select script: "
)

:: Map selection number to script name and path
for %%I in (!CHOICE!) do set "SCRIPT=!SCRIPT_%%I!"
for %%I in (!CHOICE!) do set "SCRIPT_FILE=!SPATH_%%I!"

if not defined SCRIPT (
    echo Invalid selection.
    pause
    exit /b 1
)

set "OUTPUT_FILE=!GAME_OUTPUT_DIR!\!SCRIPT!_output.log"

if not exist "!SCRIPT_FILE!" (
    echo Script not found: !SCRIPT_FILE!
    pause
    exit /b 1
)

:: Detect python host mode from @python tag
set "PYTHON_MODE="
findstr /C:"@python" "!SCRIPT_FILE!" >nul 2>&1
if not errorlevel 1 set "PYTHON_MODE=1"

:: Detect auto mode from @auto tag in script
set "AUTO_MODE="
findstr /C:"@auto" "!SCRIPT_FILE!" >nul 2>&1
if not errorlevel 1 set "AUTO_MODE=1"

echo.
echo Running: !SCRIPT!.js
echo Output:  !OUTPUT_FILE!
if defined PYTHON_MODE (
    echo Mode:    python host
) else if defined AUTO_MODE (
    echo Mode:    auto
) else (
    echo Mode:    continuous - Ctrl+C to stop
)
echo.

if defined PYTHON_MODE (
    set "HOST_FILE=!GAME_SCRIPT_DIR!\!SCRIPT!_host.py"
    if not exist "!HOST_FILE!" (
        echo Python host not found: !HOST_FILE!
        pause
        exit /b 1
    )
    python "!HOST_FILE!"
) else if defined AUTO_MODE (
    frida -n "!GAME_PROC!" -l "!SCRIPT_FILE!" -o "!OUTPUT_FILE!" --no-auto-reload
) else (
    frida -n "!GAME_PROC!" -l "!SCRIPT_FILE!" -o "!OUTPUT_FILE!"
)

echo.
echo Done. Output saved to: !OUTPUT_FILE!
pause
exit /b 0

:: ---- Subroutine: extract @auto and @desc tags from a script ----
:: Usage: call :get_info INDEX
:: Sets MODE_INDEX and DESC_INDEX variables
:get_info
set "_IDX=%1"
set "MODE_%_IDX%=interactive"
set "DESC_%_IDX%="
set "_FILE=!SPATH_%_IDX%!"
findstr /C:"@python" "!_FILE!" >nul 2>&1
if not errorlevel 1 set "MODE_%_IDX%=python"
findstr /C:"@auto" "!_FILE!" >nul 2>&1
if not errorlevel 1 if not "!MODE_%_IDX%!"=="python" set "MODE_%_IDX%=auto"
for /f "delims=" %%L in ('findstr /C:"@desc" "!_FILE!" 2^>nul') do set "_DESCLINE=%%L"
if defined _DESCLINE (
    set "_DESCLINE=!_DESCLINE:*@desc =!"
    set "DESC_%_IDX%=!_DESCLINE!"
)
set "_DESCLINE="
exit /b 0
