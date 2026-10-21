@echo off
setlocal
set "SOURCE=%~dp0"
set "OUTPUT=%~dp0..\..\install\native"
if not exist "%OUTPUT%" mkdir "%OUTPUT%"
pushd "%OUTPUT%"
cl /nologo /W4 /std:c++17 /EHsc /MT /LD /Fe:FFXLauncherNative.dll "%SOURCE%launcher_native.cpp" "%SOURCE%..\native\speech.cpp" "%SOURCE%..\native\log_capture.cpp" "%SOURCE%..\native\speech_capture.cpp" "%SOURCE%..\native\speech_queue.cpp" "%SOURCE%..\native\speech_domains.cpp" "%SOURCE%..\native\prism_backend.cpp" "%SOURCE%..\native\log_history.cpp" "%SOURCE%..\native\settings.cpp" /link /DEF:"%SOURCE%launcher_native.def" user32.lib
if errorlevel 1 (
    popd
    exit /b 1
)
cl /nologo /W4 /std:c++17 /EHa /clr /MD /LD /Fe:FFXLauncherReader.dll "%SOURCE%launcher_reader.cpp"
set "BUILD_RC=%ERRORLEVEL%"
popd
exit /b %BUILD_RC%
