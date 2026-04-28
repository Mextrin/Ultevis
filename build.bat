@echo off
set BUILD_DIR=build
set EXE_DIR=build/Airchestra_artefacts/Debug/Airchestra.exe
set VCPKG_ROOT=%~dp0vcpkg
set VCPKG_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

if "%~1"=="" (
    echo No build command provided.
    call :show-usage
    exit /b 1
)

if /I "%~1"=="build-all" (
    call :build-all
    if errorlevel 1 exit /b 1
    exit /b 0
)
if /I "%~1"=="compile" (
    call :compile-app
    if errorlevel 1 exit /b 1
    exit /b 0
)
if /I "%~1"=="run" (
    call :run-app
    exit /b 0
)
if /I "%~1"=="compile-and-run" (
    call :compile-app
    if errorlevel 1 exit /b 1
    call :run-app
    exit /b 0
)
if /I "%~1"=="clean" (
    call :clean-all 
    exit /b 0
)
if /I "%~1"=="clean-build-run" (
    call :clean-all
    call :build-all
    if errorlevel 1 exit /b 1
    call :run-app
    exit /b 0
)

echo Unknown build command: %~1
call :show-usage
exit /b 1

:show-usage
    echo.
    echo Available build commands from README.md:
    echo   build.bat build-all         Full setup and build
    echo   build.bat compile           Compile existing build
    echo   build.bat run               Run compiled executable
    echo   build.bat compile-and-run   Compile and run
    echo   build.bat clean             Remove the build folder
    echo.
exit /b

:build-all
    git submodule update --init --recursive

    if not exist ./vcpkg (
        git clone https://github.com/microsoft/vcpkg ./vcpkg
        cd vcpkg
        call .\bootstrap-vcpkg.bat

        cd ..
    )

    "%VCPKG_ROOT%\vcpkg.exe" install --triplet x64-windows
    if errorlevel 1 exit /b 1
    cmake -B %BUILD_DIR% -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN_FILE%" -DVCPKG_TARGET_TRIPLET=x64-windows
    if errorlevel 1 exit /b 1
    call :compile-app
exit /b

:compile-app
    if not exist "%BUILD_DIR%\CMakeCache.txt" (
        echo Build directory is not configured. Run build.bat build-all first.
        exit /b 1
    )
    cmake --build %BUILD_DIR%
    if errorlevel 1 exit /b 1
    call :copy-qt-platform-plugin
exit /b

:run-app
    set "ULTEVIS_LAUNCH_HAND_DETECTOR=1"
    set "ULTEVIS_HAND_DETECTOR_SCRIPT=%~dp0src\mediapipe\hand_detector.py"
    "./%EXE_DIR%"
exit /b

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
    set "QT_IMAGEFORMATS_DIR=%~dp0vcpkg_installed\x64-windows\debug\Qt6\plugins\imageformats"
    set "QT_QML_DIR=%~dp0vcpkg_installed\x64-windows\debug\Qt6\qml"
    set "QT_DEBUG_BIN=%~dp0vcpkg_installed\x64-windows\debug\bin"
    for %%F in ("%EXE_DIR%") do set "APP_OUTPUT_DIR=%%~dpF"
    if not exist "%APP_OUTPUT_DIR%platforms" mkdir "%APP_OUTPUT_DIR%platforms"
    if not exist "%APP_OUTPUT_DIR%imageformats" mkdir "%APP_OUTPUT_DIR%imageformats"
    copy /Y "%QT_WINDOWS_PLUGIN%" "%APP_OUTPUT_DIR%platforms\qwindowsd.dll"
    xcopy /E /I /Y "%QT_IMAGEFORMATS_DIR%" "%APP_OUTPUT_DIR%imageformats"
    xcopy /E /I /Y "%QT_QML_DIR%" "%APP_OUTPUT_DIR%qml"
    copy /Y "%QT_DEBUG_BIN%\Qt6*d.dll" "%APP_OUTPUT_DIR%"
    copy /Y "%QT_DEBUG_BIN%\jpeg62.dll" "%APP_OUTPUT_DIR%"
    copy /Y "%QT_DEBUG_BIN%\turbojpeg.dll" "%APP_OUTPUT_DIR%"
    > "%APP_OUTPUT_DIR%qt.conf" echo [Paths]
    >> "%APP_OUTPUT_DIR%qt.conf" echo Plugins = .
    >> "%APP_OUTPUT_DIR%qt.conf" echo Qml2Imports = qml
exit /b
