@echo off
setlocal enabledelayedexpansion

set BUILD_DIR=build
set BUILD_CONFIG=Debug
set EXE_DIR=%BUILD_DIR%\Airchestra_artefacts\%BUILD_CONFIG%\Airchestra.exe
set QT_PREFIX=C:/Qt/6.11.0/msvc2022_64
set HAND_DETECTOR_SCRIPT=%~dp0src\mediapipe\hand_detector.py

if "%1"=="build-all" (
    git submodule update --init --recursive

    if not exist vcpkg (
        git clone https://github.com/microsoft/vcpkg ./vcpkg
        cd vcpkg
        call bootstrap-vcpkg.bat
        cd ..
    )

    if exist %BUILD_DIR% rd /s /q %BUILD_DIR%
    cmake -B %BUILD_DIR% -DCMAKE_PREFIX_PATH="%QT_PREFIX%" -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_FLAGS="/Zc:__cplusplus /EHsc"
    cmake --build %BUILD_DIR% --config %BUILD_CONFIG%
    exit /b
)

if "%1"=="compile" (
    cmake --build %BUILD_DIR% --config %BUILD_CONFIG%
    exit /b
)

if "%1"=="run" (
    set "PATH=C:\Qt\6.11.0\msvc2022_64\bin;!PATH!"
    set "ULTEVIS_LAUNCH_HAND_DETECTOR=0"
    set "ULTEVIS_HAND_DETECTOR_SCRIPT=!HAND_DETECTOR_SCRIPT!"

    echo Running: "!EXE_DIR!"
    echo Hand detector script: "!ULTEVIS_HAND_DETECTOR_SCRIPT!"

    "!EXE_DIR!"
    echo Program exited with code !ERRORLEVEL!
    pause
    exit /b
)

if "%1"=="compile-and-run" (
    cmake --build %BUILD_DIR% --config %BUILD_CONFIG%

    set "PATH=C:\Qt\6.11.0\msvc2022_64\bin;!PATH!"
    set "ULTEVIS_LAUNCH_HAND_DETECTOR=1"
    set "ULTEVIS_HAND_DETECTOR_SCRIPT=!HAND_DETECTOR_SCRIPT!"

    "!EXE_DIR!"
    pause
    exit /b
)

if "%1"=="clean" (
    if exist %BUILD_DIR% rd /s /q %BUILD_DIR%
    exit /b
)

if "%1"=="clean-build-run" (
    if exist %BUILD_DIR% rd /s /q %BUILD_DIR%

    cmake -B %BUILD_DIR% -DCMAKE_PREFIX_PATH="%QT_PREFIX%" -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_FLAGS="/Zc:__cplusplus /EHsc"
    cmake --build %BUILD_DIR% --config %BUILD_CONFIG%

    set "PATH=C:\Qt\6.11.0\msvc2022_64\bin;!PATH!"
    set "ULTEVIS_LAUNCH_HAND_DETECTOR=1"
    set "ULTEVIS_HAND_DETECTOR_SCRIPT=!HAND_DETECTOR_SCRIPT!"

    "!EXE_DIR!"
    pause
    exit /b
)

echo Usage: build.bat {build-all^|compile^|run^|compile-and-run^|clean^|clean-build-run}
exit /b 1