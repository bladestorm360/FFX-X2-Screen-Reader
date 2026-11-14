@echo off
setlocal
set "PROXY_DIR=%~dp0"
set "REPO_ROOT=%~dp0..\.."
set "DEV_DEFINE=/DFFX_DEV_UTILITIES"
set "DO_DEPLOY="
:arguments
if "%~1"=="" goto configured
if /I "%~1"=="--deploy" set "DO_DEPLOY=1"
rem --no-deploy is accepted and ignored: deploying is opt-in.
if /I "%~1"=="--release" set "DEV_DEFINE="
if /I "%~1"=="--test-build" set "DEV_DEFINE="
shift /1
goto arguments
:configured
call "%REPO_ROOT%\tools\env.bat"
if errorlevel 1 exit /b 1

:: Production is entirely native. JavaScript remains an offline RE reference.
call "%VCVARSALL%" x86 >nul
if errorlevel 1 exit /b 1
set "HOOK_ROOT=%MINHOOK_ROOT%"
if not exist "%HOOK_ROOT%\bin\MinHook.x86.lib" (
    echo ERROR: x86 MinHook dependency missing at "%HOOK_ROOT%".
    exit /b 1
)
if not exist "%REPO_ROOT%\install\native" mkdir "%REPO_ROOT%\install\native"
call "%PROXY_DIR%..\native\build_sdl3.bat"
if errorlevel 1 exit /b 1
pushd "%PROXY_DIR%..\native"
rc /nologo /fo "%REPO_ROOT%\install\native\audio_assets.res" audio_assets.rc
set "BUILD_RC=%ERRORLEVEL%"
popd
if not "%BUILD_RC%"=="0" exit /b %BUILD_RC%
rc /nologo /fo "%REPO_ROOT%\install\native\version.res" "%PROXY_DIR%version.rc"
if errorlevel 1 exit /b 1
pushd "%REPO_ROOT%\install\native"
cl /nologo /W4 /O2 /std:c++17 /EHa /MT /LD %DEV_DEFINE% /I"%HOOK_ROOT%\include" /I"%SDL3_SOURCE%\include" /Fe:dinput8.dll "%PROXY_DIR%dinput8_proxy.cpp" "%PROXY_DIR%..\native\*.cpp" audio_assets.res version.res /link /MAP:dinput8.map /DEF:"%PROXY_DIR%dinput8.def" "%HOOK_ROOT%\bin\MinHook.x86.lib" user32.lib gdi32.lib dxguid.lib advapi32.lib
set "BUILD_RC=%ERRORLEVEL%"
popd
if not "%BUILD_RC%"=="0" exit /b %BUILD_RC%
copy /Y "%REPO_ROOT%\install\native\dinput8.dll" "%PROXY_DIR%dinput8.dll" >nul
if errorlevel 1 exit /b 1
copy /Y "%HOOK_ROOT%\bin\MinHook.x86.dll" "%REPO_ROOT%\install\native\MinHook.x86.dll" >nul
if errorlevel 1 exit /b 1
pushd "%REPO_ROOT%\install\native"
:: Keep the verifier at a different image base so the game can map at 00400000.
cl /nologo /W4 /EHsc /MT /Fe:profile_preflight.exe "%PROXY_DIR%profile_preflight.cpp" /link /BASE:0x30000000 /DYNAMICBASE:NO
set "BUILD_RC=%ERRORLEVEL%"
popd
if not "%BUILD_RC%"=="0" exit /b %BUILD_RC%
echo Native x86 build OK.
call "%PROXY_DIR%..\launcher\build_launcher.bat"
if errorlevel 1 exit /b 1
if not defined DO_DEPLOY exit /b 0
call "%PROXY_DIR%deploy.bat"
exit /b %ERRORLEVEL%
