@echo off
setlocal
set "TEST_SRC=%~dp0"
set "REPO_ROOT=%~dp0..\..\.."
call "%REPO_ROOT%\Docs\Scripts\evidence_root.bat"
if errorlevel 1 exit /b 1
call "%REPO_ROOT%\..\..\..\dev_env.bat"
if not defined VCVARSALL exit /b 1
call "%VCVARSALL%" x86 >nul
if not "%errorlevel%"=="0" exit /b 1
set "TEST_OUT=%FFX_EVIDENCE_ROOT%\test-build\sdl-owner-tests"
if not exist "%TEST_OUT%" mkdir "%TEST_OUT%"
pushd "%TEST_SRC%.."
rc /nologo /fo "%TEST_OUT%\audio_assets.res" audio_assets.rc
if not "%errorlevel%"=="0" goto failed
popd
pushd "%TEST_OUT%"
rem Fake SDK and host are isolated here; no real game, audio device or SDK probe.
cl /nologo /W4 /WX /EHsc /std:c++17 /MT /I"%SDL3_SOURCE%\include" /LD /Fe:SDL3.dll "%TEST_SRC%fake_sdl_owner.cpp" /link /DEF:"%TEST_SRC%fake_sdl_owner.def"
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /WX /EHa /std:c++17 /MT /DFFX_READER_TEST /DFFX_SDL_OWNER_TEST /I"%SDL3_SOURCE%\include" /Fe:sdl_owner_fixture.exe "%TEST_SRC%sdl_owner_fixture.cpp" "%TEST_SRC%..\controller.cpp" "%TEST_SRC%..\physical_gamepad.cpp" "%TEST_SRC%..\sdl_runtime.cpp" "%TEST_SRC%..\nav_audio.cpp" audio_assets.res /link user32.lib
if not "%errorlevel%"=="0" goto failed
for %%C in (normal init audio version owner preloaded pads) do (
    sdl_owner_fixture.exe %%C
    if errorlevel 1 goto failed
)
popd
exit /b 0
:failed
popd
exit /b 1
