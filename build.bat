@echo off
set BUILD_DIR=build
set EXE_DIR=build/Airchestra_artefacts/Debug/Airchestra.exe

if "%1"=="configure" (
    git clone https://github.com/microsoft/vcpkg ./vcpkg
    cd vcpkg
    /bootstrap-vcpkg.bat
    cd ..
    cmake -B %BUILD_DIR% -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
)
if "%1"=="build" cmake --build %BUILD_DIR%
if "%1"=="run" "./%EXE_DIR%"
if "%1"=="clean" (
    rd /s /q %BUILD_DIR%
)
if "%1"=="clean-all" (
    rd /s /q %BUILD_DIR%
    rd /s /q vcpkg
)
if "%1"=="rebuild" (
    rd /s /q %BUILD_DIR%
    cmake -B %BUILD_DIR%
    cmake --build %BUILD_DIR%
)