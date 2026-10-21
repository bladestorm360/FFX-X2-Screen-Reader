@echo off
setlocal
set "REPO_ROOT=%~dp0..\..\.."
call "%REPO_ROOT%\Docs\Scripts\evidence_root.bat"
if errorlevel 1 exit /b 1
call "%~dp0..\..\..\..\..\..\dev_env.bat"
call "%VCVARSALL%" x86 >nul
if errorlevel 1 exit /b 1
set "TEST_BUILD=%FFX_EVIDENCE_ROOT%\test-build"
if not exist "%TEST_BUILD%" mkdir "%TEST_BUILD%"
pushd "%TEST_BUILD%"
cl /nologo /W4 /EHsc /std:c++17 /MT /DFFX_READER_TEST /Fe:accessibility_fixture.exe "%~dp0accessibility_fixture.cpp" "%~dp0..\mod_menu.cpp" "%~dp0..\settings.cpp" "%~dp0..\controller_bindings.cpp" /link user32.lib
if errorlevel 1 exit /b 1
accessibility_fixture.exe
set "TASK_RESULT=%errorlevel%"
popd
exit /b %TASK_RESULT%
