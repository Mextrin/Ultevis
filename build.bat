@echo off
set BUILD_DIR=build
set EXE_DIR=build/Airchestra_artefacts/Release/Airchestra.exe
set VCPKG_ROOT=%~dp0vcpkg
set VCPKG_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
rem Use a short buildtrees path to avoid Windows MAX_PATH (260 chars) failures
rem when building qtdeclarative (fluentwinui3 plugin generates very long .moc paths).
set VCPKG_BUILDTREES_ROOT=C:\vcbt

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
    if defined BUILD_FAILED exit /b 1
    if errorlevel 1 exit /b 1
    exit /b 0
)
if /I "%~1"=="run" (
    call :run-app
    exit /b 0
)
if /I "%~1"=="compile-and-run" (
    call :compile-app
    if defined BUILD_FAILED exit /b 1
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
    rem Initialize only the project submodules. `vcpkg` is fetched manually below
    rem (it isn't in .gitmodules), so we skip the global recursive update.
    git submodule update --init --recursive JUCE sfizz

    if not exist ./vcpkg (
        git clone https://github.com/microsoft/vcpkg ./vcpkg
        cd vcpkg
        call .\bootstrap-vcpkg.bat

        cd ..
    )

    if not exist "%VCPKG_BUILDTREES_ROOT%" mkdir "%VCPKG_BUILDTREES_ROOT%"
    rem CMake's vcpkg manifest mode also runs `vcpkg install`, so pass the same
    rem buildtrees root via VCPKG_INSTALL_OPTIONS to avoid Windows MAX_PATH issues.
    rem Use forward slashes so CMake doesn't choke on backslash escapes.
    set "VCPKG_BUILDTREES_ROOT_FWD=%VCPKG_BUILDTREES_ROOT:\=/%"
    set "VCPKG_INSTALLED_DIR_FWD=%~dp0vcpkg_installed"
    set "VCPKG_INSTALLED_DIR_FWD=%VCPKG_INSTALLED_DIR_FWD:\=/%"
    "%VCPKG_ROOT%\vcpkg.exe" install --triplet x64-windows --x-buildtrees-root="%VCPKG_BUILDTREES_ROOT%"
    if errorlevel 1 exit /b 1
    cmake -B %BUILD_DIR% -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN_FILE%" -DVCPKG_TARGET_TRIPLET=x64-windows "-DVCPKG_INSTALL_OPTIONS=--x-buildtrees-root=%VCPKG_BUILDTREES_ROOT_FWD%" "-DVCPKG_INSTALLED_DIR=%VCPKG_INSTALLED_DIR_FWD%"
    if errorlevel 1 exit /b 1
    call :compile-app
exit /b

:compile-app
    if not exist "%BUILD_DIR%\CMakeCache.txt" (
        echo Build directory is not configured. Run build.bat build-all first.
        set "BUILD_FAILED=1"
        exit /b 1
    )
    rem Visual Studio is multi-config; pick Release explicitly to match EXE_DIR.
    cmake --build %BUILD_DIR% --config Release
    if errorlevel 1 (
        set "BUILD_FAILED=1"
        exit /b 1
    )
    call :copy-qt-platform-plugin
    if defined BUILD_FAILED exit /b 1
    if errorlevel 1 (
        set "BUILD_FAILED=1"
        exit /b 1
    )
exit /b

:run-app
    set "ULTEVIS_LAUNCH_HAND_DETECTOR=1"
    for %%F in ("%EXE_DIR%") do set "APP_OUTPUT_DIR=%%~dpF"
    set "ULTEVIS_HAND_DETECTOR_SCRIPT=%APP_OUTPUT_DIR%mediapipe\hand_detector.py"
    "./%EXE_DIR%"
exit /b

:clean-all
    call :rename-and-delete "%BUILD_DIR%"
    call :rename-and-delete "vcpkg"
    call :rename-and-delete "vcpkg_installed"
    call :rename-and-delete "%VCPKG_BUILDTREES_ROOT%"
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
    rem Release-configuration paths. In vcpkg's Qt layout, plugins live under
    rem `<triplet>\Qt6\plugins\...` and QML imports under `<triplet>\Qt6\qml`.
    set "QT_PLATFORMS_DIR=%~dp0vcpkg_installed\x64-windows\Qt6\plugins\platforms"
    set "QT_IMAGEFORMATS_DIR=%~dp0vcpkg_installed\x64-windows\Qt6\plugins\imageformats"
    set "QT_QML_DIR=%~dp0vcpkg_installed\x64-windows\Qt6\qml"
    set "QT_RELEASE_BIN=%~dp0vcpkg_installed\x64-windows\bin"
    for %%F in ("%EXE_DIR%") do set "APP_OUTPUT_DIR=%%~dpF"
    if not exist "%APP_OUTPUT_DIR%" (
        echo [copy-qt] Output directory does not exist: %APP_OUTPUT_DIR%
        set "BUILD_FAILED=1"
        exit /b 1
    )
    if not exist "%QT_PLATFORMS_DIR%\qwindows.dll" (
        echo [copy-qt] Missing Qt platform plugin: %QT_PLATFORMS_DIR%\qwindows.dll
        echo [copy-qt] Re-run `build.bat build-all` to install Qt via vcpkg.
        set "BUILD_FAILED=1"
        exit /b 1
    )
    call :copy-runtime-dir "%QT_PLATFORMS_DIR%"    "%APP_OUTPUT_DIR%platforms"    "Qt platforms"
    if errorlevel 1 exit /b 1
    call :copy-runtime-dir "%QT_IMAGEFORMATS_DIR%" "%APP_OUTPUT_DIR%imageformats" "Qt imageformats"
    if errorlevel 1 exit /b 1
    call :copy-runtime-dir "%QT_QML_DIR%"          "%APP_OUTPUT_DIR%qml"          "Qt QML"
    if errorlevel 1 exit /b 1
    call :copy-runtime-dir "%~dp0src\mediapipe"    "%APP_OUTPUT_DIR%mediapipe"    "MediaPipe scripts"
    if errorlevel 1 exit /b 1
    call :copy-runtime-dir "%~dp0Instruments"      "%APP_OUTPUT_DIR%Instruments"  "Instruments"
    if errorlevel 1 exit /b 1
    copy /Y "%QT_RELEASE_BIN%\Qt6*.dll"   "%APP_OUTPUT_DIR%" >nul
    copy /Y "%QT_RELEASE_BIN%\jpeg62.dll" "%APP_OUTPUT_DIR%" >nul
    copy /Y "%QT_RELEASE_BIN%\turbojpeg.dll" "%APP_OUTPUT_DIR%" >nul
    > "%APP_OUTPUT_DIR%qt.conf" echo [Paths]
    >> "%APP_OUTPUT_DIR%qt.conf" echo Plugins = .
    >> "%APP_OUTPUT_DIR%qt.conf" echo Qml2Imports = qml
exit /b

:copy-runtime-dir
    if not exist "%~1" (
        echo [copy-runtime] Missing source directory for %~3: %~1
        set "BUILD_FAILED=1"
        exit /b 1
    )
    robocopy "%~1" "%~2" /E /R:2 /W:1 /NFL /NDL /NP /XD .git __pycache__ >nul
    if %ERRORLEVEL% GEQ 8 (
        echo [copy-runtime] Failed to copy %~3 from %~1 to %~2
        set "BUILD_FAILED=1"
        exit /b 1
    )
exit /b 0
