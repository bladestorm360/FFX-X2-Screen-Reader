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
set "TEST_OUT=%FFX_EVIDENCE_ROOT%\test-build\input-dialog-focus-tests"
if not exist "%TEST_OUT%" mkdir "%TEST_OUT%"
pushd "%TEST_OUT%"
rem Production settings focus logic runs with synthetic Win32 identity calls.
cl /nologo /W4 /WX /EHa /std:c++17 /MT /Fe:input_dialog_focus_fixture.exe "%TEST_SRC%input_dialog_focus_fixture.cpp" /link user32.lib gdi32.lib
if not "%errorlevel%"=="0" goto failed
input_dialog_focus_fixture.exe
if errorlevel 1 goto failed
popd
exit /b 0
:failed
popd
exit /b 1
