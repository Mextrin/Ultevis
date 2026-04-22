@echo off
set BUILD_DIR=build
set EXE_DIR=build/Airchestra_artefacts/Debug/Airchestra.exe

if "%1"=="build-all" (
    git submodule update --init --recursive

    if not exist ./vcpkg (
        git clone https://github.com/microsoft/vcpkg ./vcpkg
        cd vcpkg
        /bootstrap-vcpkg.bat
        cd ..
    )

    cmake -B %BUILD_DIR%
    cmake --build %BUILD_DIR%
)
if "%1"=="compile" cmake --build %BUILD_DIR%
if "%1"=="run" (
    set "ULTEVIS_LAUNCH_HAND_DETECTOR=1"
    set "ULTEVIS_HAND_DETECTOR_SCRIPT=%~dp0src\mediapipe\hand_detector.py"
    "./%EXE_DIR%"
)
if "%1"=="compile-and-run" (
    cmake --build %BUILD_DIR%
    set "ULTEVIS_LAUNCH_HAND_DETECTOR=1"
    set "ULTEVIS_HAND_DETECTOR_SCRIPT=%~dp0src\mediapipe\hand_detector.py"
    "./%EXE_DIR%"
)
if "%1"=="clean" (
    call :clean-all 
)
if "%1"=="clean-build-run" (
    rd /s /q %BUILD_DIR%
    cmake -B %BUILD_DIR%
    cmake --build %BUILD_DIR%
    set "ULTEVIS_LAUNCH_HAND_DETECTOR=1"
    set "ULTEVIS_HAND_DETECTOR_SCRIPT=%~dp0src\mediapipe\hand_detector.py"
    "./%EXE_DIR%"
)
exit

:clean-all
    rd /s /q %BUILD_DIR%
exit \b
