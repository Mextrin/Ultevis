@echo off
set BUILD_DIR=build
set EXE_DIR=build/Airchestra_artefacts/Debug/Airchestra.exe
set VCPKG_ROOT=%~dp0vcpkg
set VCPKG_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

if "%1"=="build-all" (
    git submodule update --init --recursive

    if not exist ./vcpkg (
        git clone https://github.com/microsoft/vcpkg ./vcpkg
        cd vcpkg
        call .\bootstrap-vcpkg.bat

        cd ..
    )

    "%VCPKG_ROOT%\vcpkg.exe" install --triplet x64-windows
    cmake -B %BUILD_DIR% -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN_FILE%" -DVCPKG_TARGET_TRIPLET=x64-windows
    cmake --build %BUILD_DIR%
    call :copy-qt-platform-plugin
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
    call :rename-and-delete "%BUILD_DIR%"
    cmake -B %BUILD_DIR% -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN_FILE%" -DVCPKG_TARGET_TRIPLET=x64-windows
    cmake --build %BUILD_DIR%
    set "ULTEVIS_LAUNCH_HAND_DETECTOR=1"
    set "ULTEVIS_HAND_DETECTOR_SCRIPT=%~dp0src\mediapipe\hand_detector.py"
    "./%EXE_DIR%"
)
exit

:clean-all
    call :rename-and-delete "%BUILD_DIR%"
    call :rename-and-delete "vcpkg"
    call :rename-and-delete "vcpkg_installed"
exit /b

:rename-and-delete
    if not exist "%~1" exit /b
    set "DELETE_NAME=%~nx1-delete"
    set "DELETE_TARGET=%~1-delete"
    if not exist "%DELETE_TARGET%" goto rename-and-delete-ready
    set "DELETE_SUFFIX=%RANDOM%"
    set "DELETE_NAME=%~nx1-delete-%DELETE_SUFFIX%"
    set "DELETE_TARGET=%~1-delete-%DELETE_SUFFIX%"

:rename-and-delete-ready
    ren "%~1" "%DELETE_NAME%"
    start "" /b cmd /c rd /s /q "%DELETE_TARGET%"
exit /b

:copy-qt-platform-plugin
    set "QT_WINDOWS_PLUGIN=%~dp0vcpkg_installed\x64-windows\debug\Qt6\plugins\platforms\qwindowsd.dll"
    set "QT_QML_DIR=%~dp0vcpkg_installed\x64-windows\debug\Qt6\qml"
    set "QT_DEBUG_BIN=%~dp0vcpkg_installed\x64-windows\debug\bin"
    for %%F in ("%EXE_DIR%") do set "APP_OUTPUT_DIR=%%~dpF"
    if not exist "%APP_OUTPUT_DIR%platforms" mkdir "%APP_OUTPUT_DIR%platforms"
    copy /Y "%QT_WINDOWS_PLUGIN%" "%APP_OUTPUT_DIR%platforms\qwindowsd.dll"
    xcopy /E /I /Y "%QT_QML_DIR%" "%APP_OUTPUT_DIR%qml"
    copy /Y "%QT_DEBUG_BIN%\Qt6*d.dll" "%APP_OUTPUT_DIR%"
    > "%APP_OUTPUT_DIR%qt.conf" echo [Paths]
    >> "%APP_OUTPUT_DIR%qt.conf" echo Plugins = .
    >> "%APP_OUTPUT_DIR%qt.conf" echo Qml2Imports = qml
exit /b
