@echo off
set BUILD_DIR=build
set EXE_DIR=build/Airchestra_artefacts/Debug/Airchestra.exe

if "%1"=="configure" cmake -B %BUILD_DIR%
if "%1"=="build" cmake --build %BUILD_DIR%
if "%1"=="run" "./%EXE_DIR%"
if "%1"=="clean" rd /s /q %BUILD_DIR%
if "%1"=="rebuild" (
    rd /s /q %BUILD_DIR%
    cmake -B %BUILD_DIR%
    cmake --build %BUILD_DIR%
)