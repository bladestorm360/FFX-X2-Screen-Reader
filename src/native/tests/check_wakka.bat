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
cl /nologo /W4 /EHa /std:c++17 /MT /DFFX_READER_TEST /Fe:wakka_fixture.exe "%~dp0wakka_fixture.cpp" "%~dp0..\wakka.cpp" "%~dp0..\wakka_identity.cpp" "%~dp0..\owned_retirement.cpp" "%~dp0..\settings.cpp" "%~dp0..\profile.cpp" /link user32.lib advapi32.lib
if errorlevel 1 exit /b 1
set "NATIVE_EVIDENCE=%FFX_EVIDENCE_ROOT%\offline-output\wakka_native_20261008_213012"
set "VARIANT_EVIDENCE=%FFX_EVIDENCE_ROOT%\offline-output\wakka_variants_20261009_144514"
wakka_fixture.exe "%VARIANT_EVIDENCE%\magic_0697.dll" "%VARIANT_EVIDENCE%\magic_0698.dll" "%NATIVE_EVIDENCE%\magic_0565.dll" "%NATIVE_EVIDENCE%\magic_0644.dll" "%VARIANT_EVIDENCE%\magic_0691.dll" "%VARIANT_EVIDENCE%\magic_0692.dll" "%VARIANT_EVIDENCE%\magic_0693.dll" "%VARIANT_EVIDENCE%\magic_0694.dll" "%VARIANT_EVIDENCE%\magic_0695.dll" "%VARIANT_EVIDENCE%\magic_0696.dll"
set "TASK_RESULT=%errorlevel%"
popd
exit /b %TASK_RESULT%
