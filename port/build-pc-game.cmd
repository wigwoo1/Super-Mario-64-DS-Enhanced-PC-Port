@echo off
setlocal
set "TEMP=%~dp0..\build\msvc-tmp"
set "TMP=%TEMP%"
set "PATH=%USERPROFILE%\.cache\codex-runtimes\codex-primary-runtime\dependencies\python;%USERPROFILE%\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\Scripts;%PATH%"
if not exist "%TEMP%" mkdir "%TEMP%"
if not exist "%~dp0..\build\pc-game-link-history.log" type nul > "%~dp0..\build\pc-game-link-history.log"
if exist "%~dp0..\build\pc-game-build.log" findstr /C:"unresolved external symbol" "%~dp0..\build\pc-game-build.log" >> "%~dp0..\build\pc-game-link-history.log"
copy /b "%~dp0..\build\pc-game-link-history.log" +,, >nul
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build "%~dp0..\build\port" --target sm64ds_pc -j 4 > "%~dp0..\build\pc-game-build.log" 2>&1
set "BUILD_STATUS=%ERRORLEVEL%"
type "%~dp0..\build\pc-game-build.log" | findstr /C:"FAILED:" /C:"error C" /C:"error LNK" /C:"fatal error" /C:"Linking CXX executable" /C:"ninja: build stopped"
echo BUILD_STATUS=%BUILD_STATUS%
exit /b %BUILD_STATUS%
