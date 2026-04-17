@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
cd /d "%~dp0"
cl /LD /Fe:dinput8.dll dinput8_proxy.cpp /link /DEF:dinput8.def user32.lib
echo.
if exist dinput8.dll (
    echo SUCCESS: dinput8.dll created
) else (
    echo FAILED: dinput8.dll was not created
)
pause
