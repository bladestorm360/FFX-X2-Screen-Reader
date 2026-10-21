@echo off
REM Production is native C++. --prototype builds the offline Frida reference.

setlocal
if /I "%~1"=="--prototype" goto prototype
call "%~dp0..\proxy\build_dinput8.bat" %*
exit /b %ERRORLEVEL%
:prototype
set BUILD_DIR=%~dp0
set SRC_DIR=%BUILD_DIR%..\
set REPO_DIR=%BUILD_DIR%..\..\

if not exist "%BUILD_DIR%node_modules" (
    echo Installing frida-compile...
    pushd "%BUILD_DIR%"
    call npm install --no-audit --no-fund --loglevel=error
    if errorlevel 1 (
        echo npm install failed.
        popd
        exit /b 1
    )
    popd
)

if not exist "%REPO_DIR%install" mkdir "%REPO_DIR%install"

pushd "%SRC_DIR%"
call npx --prefix build --no-install frida-compile index.js -o "..\install\ffx_sr.js" -B iife -T none
set BUILD_RC=%ERRORLEVEL%
popd

if %BUILD_RC% NEQ 0 (
    echo frida-compile failed with code %BUILD_RC%.
    exit /b %BUILD_RC%
)

echo Build OK: install\ffx_sr.js

REM Prototypes never become production readers through this entry point.
exit /b 0
