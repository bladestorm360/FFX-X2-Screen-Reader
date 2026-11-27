@echo off
rem Resolves the toolchain and dependency locations for the build and test
rem scripts. Variables that are already set are left alone. Intended to be
rem called (not started) so the settings reach the calling script.
rem Sets VCVARSALL and resolves optional CMAKE_EXE, MINHOOK_ROOT and SDL3_SOURCE.
rem Build stages validate only the dependencies they actually consume.
for %%I in ("%~dp0..") do set "FFX_REPO=%%~fI"
if not defined MINHOOK_ROOT if exist "%FFX_REPO%\third_party\minhook\bin\MinHook.x86.lib" set "MINHOOK_ROOT=%FFX_REPO%\third_party\minhook"
if not defined SDL3_SOURCE if exist "%FFX_REPO%\third_party\SDL3-3.4.4\CMakeLists.txt" set "SDL3_SOURCE=%FFX_REPO%\third_party\SDL3-3.4.4"
if exist "%FFX_REPO%\..\..\..\dev_env.bat" call "%FFX_REPO%\..\..\..\dev_env.bat"

set "FFX_VS_PATH="
set "FFX_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%FFX_VSWHERE%" goto vs_done
for /f "usebackq delims=" %%I in (`"%FFX_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "FFX_VS_PATH=%%I"
:vs_done
if defined VCVARSALL goto vs_resolved
if not defined FFX_VS_PATH goto vs_resolved
if exist "%FFX_VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARSALL=%FFX_VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat"
:vs_resolved
if not exist "%VCVARSALL%" goto missing_vcvarsall

if defined CMAKE_EXE goto cmake_resolved
if not defined FFX_VS_PATH goto cmake_path
if exist "%FFX_VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_EXE=%FFX_VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if defined CMAKE_EXE goto cmake_resolved
:cmake_path
for /f "delims=" %%I in ('where cmake.exe 2^>nul') do if not defined CMAKE_EXE set "CMAKE_EXE=%%I"
:cmake_resolved
if not defined MINHOOK_ROOT set "MINHOOK_ROOT=%FFX_REPO%\third_party\minhook"
if not defined SDL3_SOURCE set "SDL3_SOURCE=%FFX_REPO%\third_party\SDL3-3.4.4"
set "FFX_VS_PATH="
set "FFX_VSWHERE="
set "FFX_REPO="
exit /b 0

:missing_vcvarsall
echo ERROR: VCVARSALL not found. Install Visual Studio 2022 with the C++ desktop workload, or set VCVARSALL.
set "FFX_VS_PATH="
set "FFX_VSWHERE="
set "FFX_REPO="
exit /b 1
