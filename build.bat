@echo off
set BUILD_DIR=build
set EXE_DIR=build/Airchestra_artefacts/Debug/Airchestra.exe

if "%1"=="build-all" (
    git clone https://github.com/microsoft/vcpkg ./vcpkg
    cd vcpkg
    /bootstrap-vcpkg.bat
    cd ..
    vcpkg/vcpkg.exe add port opencv4

    cmake -B %BUILD_DIR% -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
    cmake --build %BUILD_DIR%
)
if "%1"=="compile" cmake --build %BUILD_DIR%
if "%1"=="run" "./%EXE_DIR%"
if "%1"=="compile-and-run" (
    cmake --build %BUILD_DIR%
    "./%EXE_DIR%"
)
if "%1"=="clean" (
    call :clean-all 
)   
exit

:clean-all
    rd /s /q %BUILD_DIR%
exit \b
