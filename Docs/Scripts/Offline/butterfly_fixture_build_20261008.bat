@echo off
setlocal
call "%~dp0..\..\..\..\..\..\dev_env.bat"
call "%VCVARSALL%" x86 >nul
cd /d "%~dp0..\..\..\install\native-tests"
cl /nologo /W4 /EHsc /std:c++17 /MT /Fe:butterfly_fixture.exe "..\..\src\native\tests\butterfly_fixture.cpp" "..\..\src\native\butterfly.cpp" "..\..\src\native\atel.cpp" "..\..\src\native\atel_memory.cpp"
if errorlevel 1 exit /b 1
butterfly_fixture.exe