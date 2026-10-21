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
set "TEST_OUT=%FFX_EVIDENCE_ROOT%\test-build\sphere-jump-ui-tests"
if not exist "%TEST_OUT%" mkdir "%TEST_OUT%"
pushd "%TEST_OUT%"
rem This fixture substitutes window APIs and never starts its mocked dispatcher.
cl /nologo /W4 /WX /EHa /std:c++17 /MT /Fe:sphere_jump_ui_fixture.exe "%TEST_SRC%sphere_jump_ui_fixture.cpp" /link user32.lib gdi32.lib
if not "%errorlevel%"=="0" goto failed
sphere_jump_ui_fixture.exe
if errorlevel 1 goto failed
popd
exit /b 0
:failed
popd
exit /b 1
