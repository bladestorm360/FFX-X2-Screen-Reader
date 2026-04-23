@echo off
setlocal
set GAME_DIR=D:\Games\steamlibrary\steamapps\common\FINAL FANTASY FFX^&FFX-2 HD Remaster
set REPO_ROOT=%~dp0..\..

echo Deploying to: "%GAME_DIR%"
echo.

:: Proxy DLL
copy /Y "%~dp0dinput8.dll" "%GAME_DIR%\dinput8.dll"

:: Frida Gadget + config
copy /Y "%REPO_ROOT%\lib\frida-gadget.dll" "%GAME_DIR%\frida-gadget.dll"
copy /Y "%REPO_ROOT%\lib\frida-gadget.config" "%GAME_DIR%\frida-gadget.config"

:: Bundled mod script (frida-compile output)
copy /Y "%REPO_ROOT%\install\ffx_sr.js" "%GAME_DIR%\ffx_sr.js"

echo.
echo Deploy complete.
endlocal
