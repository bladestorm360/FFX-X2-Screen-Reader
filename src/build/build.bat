@echo off
REM Bundles src/**/*.js into install/ffx_sr.js via frida-compile,
REM then deploys to the game directory via deploy.bat.

setlocal
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

REM Deploy the rebuilt bundle. deploy.bat copies dinput8.dll, frida-gadget,
REM frida-gadget.config, and install/ffx_sr.js into the game directory.
REM The DLL + gadget copies are redundant on JS-only rebuilds but harmless.
call "%REPO_DIR%src\proxy\deploy.bat"
if errorlevel 1 (
    echo Deploy failed.
    exit /b 1
)

endlocal
