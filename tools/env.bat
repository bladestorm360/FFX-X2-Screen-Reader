@echo off
rem Resolves the toolchain and dependency locations for the build and test
rem scripts. Variables that are already set are left alone. Intended to be
rem called (not started) so the settings reach the calling script.
rem Sets: VCVARSALL CMAKE_EXE MINHOOK_ROOT SDL3_SOURCE PRISM_ROOT
rem       FFX_EVIDENCE_ROOT
rem Exits with code 1 and one ERROR line when a required item is missing.
for %%I in ("%~dp0..") do set "FFX_REPO=%%~fI"

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
if not exist "%CMAKE_EXE%" goto missing_cmake

if not defined MINHOOK_ROOT set "MINHOOK_ROOT=%FFX_REPO%\third_party\minhook"
if not exist "%MINHOOK_ROOT%\bin\MinHook.x86.lib" goto missing_minhook

if not defined SDL3_SOURCE set "SDL3_SOURCE=%FFX_REPO%\third_party\SDL3-3.4.4"
if not exist "%SDL3_SOURCE%\CMakeLists.txt" goto missing_sdl3

rem PRISM is only needed to package the installer (see prism.lock.json); the
rem build and tests use the headers in the source tree, so it is not checked.
if not defined PRISM_ROOT set "PRISM_ROOT=%FFX_REPO%\third_party\prism"

if not defined FFX_EVIDENCE_ROOT set "FFX_EVIDENCE_ROOT=%LOCALAPPDATA%\FFX-SR\evidence"
set "FFX_VS_PATH="
set "FFX_VSWHERE="
set "FFX_REPO="
exit /b 0

:missing_vcvarsall
echo ERROR: VCVARSALL not found. Install Visual Studio 2022 with the C++ desktop workload, or set VCVARSALL.
goto failed
:missing_cmake
echo ERROR: CMAKE_EXE not found. Install the Visual Studio CMake tools, or set CMAKE_EXE.
goto failed
:missing_minhook
echo ERROR: MINHOOK_ROOT not found. Run: powershell -ExecutionPolicy Bypass -File tools\fetch_deps.ps1
goto failed
:missing_sdl3
echo ERROR: SDL3_SOURCE not found. Run: powershell -ExecutionPolicy Bypass -File tools\fetch_deps.ps1
goto failed
:failed
set "FFX_VS_PATH="
set "FFX_VSWHERE="
set "FFX_REPO="
exit /b 1
