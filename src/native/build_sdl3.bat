@echo off
setlocal
set "SDL_BUILD=%~dp0..\..\install\sdl3-x86"
set "SDL_STAGE=%~dp0..\..\install\native"
if not exist "%SDL3_SOURCE%\CMakeLists.txt" (
    echo ERROR: SDL3 source missing: "%SDL3_SOURCE%".
    exit /b 1
)
if not exist "%CMAKE_EXE%" exit /b 1
"%CMAKE_EXE%" -S "%SDL3_SOURCE%" -B "%SDL_BUILD%" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
if errorlevel 1 exit /b 1
"%CMAKE_EXE%" --build "%SDL_BUILD%" --config Release --target SDL3-shared
if errorlevel 1 exit /b 1
if not exist "%SDL_STAGE%" mkdir "%SDL_STAGE%"
copy /Y "%SDL_BUILD%\SDL3.dll" "%SDL_STAGE%\SDL3.dll" >nul
if errorlevel 1 exit /b 1
copy /Y "%SDL3_SOURCE%\LICENSE.txt" "%SDL_STAGE%\SDL3_LICENSE.txt" >nul
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -Command "$bytes = [IO.File]::ReadAllBytes((Join-Path $env:SDL_STAGE 'SDL3.dll')); $offset = [BitConverter]::ToInt32($bytes,60); if ([BitConverter]::ToUInt16($bytes,$offset+4) -ne 332) { throw 'SDL3 must be x86' }"
exit /b %errorlevel%
