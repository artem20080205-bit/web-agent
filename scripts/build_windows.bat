@echo off
REM Windows build script for Web-Agent
REM Requires: vcpkg, CMake, Visual Studio

echo === Web-Agent Windows Build ===

REM Set vcpkg root if not set
if "%VCPKG_ROOT%"=="" set VCPKG_ROOT=C:\vcpkg

REM Install dependencies via vcpkg
echo Installing dependencies...
%VCPKG_ROOT%\vcpkg install curl:x64-windows nlohmann-json:x64-windows

REM Configure and build
mkdir build_win
cd build_win
cmake .. ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release

echo.
echo === Build complete ===
echo Binary: %CD%\Release\webagent.exe
echo Run:    webagent.exe --config ..\config\agent.json
