@echo off
REM Build pipeline: (1) bundle src/**/*.js -> install/ffx_sr.js via frida-compile,
REM (2) compile the dinput8 proxy DLL.

setlocal
set PROXY_DIR=%~dp0
set BUILD_DIR=%PROXY_DIR%..\build\

echo === JS bundle ===
call "%BUILD_DIR%build.bat"
if errorlevel 1 (
    echo JS bundle failed.
    exit /b 1
)

echo.
echo === dinput8.dll ===
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
cd /d "%PROXY_DIR%"
cl /LD /Fe:dinput8.dll dinput8_proxy.cpp /link /DEF:dinput8.def user32.lib
echo.
if exist dinput8.dll (
    echo SUCCESS: dinput8.dll created
) else (
    echo FAILED: dinput8.dll was not created
    exit /b 1
)

echo.
echo === Deploy ===
call "%PROXY_DIR%deploy.bat"
if errorlevel 1 (
    echo Deploy failed.
    exit /b 1
)

pause
endlocal
