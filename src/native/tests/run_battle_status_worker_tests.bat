@echo off
setlocal
set "REPO_ROOT=%~dp0..\..\.."
call "%REPO_ROOT%\Docs\Scripts\evidence_root.bat"
if errorlevel 1 exit /b 1
call "%REPO_ROOT%\..\..\..\dev_env.bat"
if not defined VCVARSALL exit /b 1
call "%VCVARSALL%" x86 >nul
if errorlevel 1 exit /b 1
set "STATUS_BUILD=%FFX_EVIDENCE_ROOT%\test-build\battle-status-worker"
if not exist "%STATUS_BUILD%" mkdir "%STATUS_BUILD%"
pushd "%STATUS_BUILD%"
cl /nologo /W4 /WX /EHsc /std:c++17 /MT /Gy /DFFX_SPEECH_WAKE_TEST /Fe:battle_status_worker_fixture.exe "%~dp0battle_status_worker_fixture.cpp" "%~dp0..\battle_status.cpp" "%~dp0..\ready_game_text.cpp" "%~dp0..\ready_text.cpp" "%~dp0..\text_plan.cpp" "%~dp0..\selection_details.cpp" "%~dp0..\speech_queue.cpp" "%~dp0..\speech_capture.cpp" "%~dp0..\speech_domains.cpp" /link user32.lib /OPT:REF
if errorlevel 1 exit /b 1
battle_status_worker_fixture.exe
exit /b %ERRORLEVEL%
