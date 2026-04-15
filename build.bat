@echo off
set BUILD_DIR=build
set EXE_DIR=build/Airchestra_artefacts/Debug/Airchestra.exe
set INSTRUMENTS_DIR=Instruments

if "%1"=="build-all" (
    call :ensure-instruments
    if errorlevel 1 exit /b 1

    git clone https://github.com/microsoft/vcpkg ./vcpkg
    cd vcpkg
    /bootstrap-vcpkg.bat
    cd ..

    cmake -B %BUILD_DIR%
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

:ensure-instruments
    if exist "%INSTRUMENTS_DIR%\" (
        echo Instruments folder already exists. Skipping download.
        exit /b 0
    )

    echo Instruments folder not found. Downloading instrument pack...
    powershell -NoProfile -ExecutionPolicy Bypass -Command ^
        "$ErrorActionPreference = 'Stop';" ^
        "$destination = Join-Path (Get-Location) 'Instruments';" ^
        "$zipPath = Join-Path $env:TEMP 'Ultevis-Instruments.zip';" ^
        "$extractPath = Join-Path $env:TEMP ('Ultevis-Instruments-' + [guid]::NewGuid());" ^
        "$downloadUrls = @(" ^
        "  'https://drive.usercontent.google.com/download?id=1DHs_jQyNGfls6j3mvyO9F8DpK8vkySLa&export=download&confirm=t'," ^
        "  'https://drive.google.com/uc?export=download&id=1DHs_jQyNGfls6j3mvyO9F8DpK8vkySLa'" ^
        ");" ^
        "New-Item -ItemType Directory -Path $extractPath | Out-Null;" ^
        "$downloaded = $false;" ^
        "foreach ($url in $downloadUrls) {" ^
        "  try {" ^
        "    Invoke-WebRequest -Uri $url -OutFile $zipPath -UseBasicParsing;" ^
        "    if ((Test-Path $zipPath) -and ((Get-Item $zipPath).Length -gt 0)) {" ^
        "      $downloaded = $true;" ^
        "      break;" ^
        "    }" ^
        "  } catch {" ^
        "  }" ^
        "}" ^
        "if (-not $downloaded) { throw 'Failed to download the Instruments archive from Google Drive.' };" ^
        "Expand-Archive -LiteralPath $zipPath -DestinationPath $extractPath -Force;" ^
        "New-Item -ItemType Directory -Path $destination -Force | Out-Null;" ^
        "$entries = Get-ChildItem -LiteralPath $extractPath -Force;" ^
        "if ($entries.Count -eq 1 -and $entries[0].PSIsContainer) {" ^
        "  $sourcePath = $entries[0].FullName;" ^
        "} else {" ^
        "  $sourcePath = $extractPath;" ^
        "}" ^
        "Get-ChildItem -LiteralPath $sourcePath -Force | ForEach-Object {" ^
        "  Copy-Item -LiteralPath $_.FullName -Destination $destination -Recurse -Force;" ^
        "};" ^
        "Remove-Item -LiteralPath $zipPath -Force -ErrorAction SilentlyContinue;" ^
        "Remove-Item -LiteralPath $extractPath -Recurse -Force -ErrorAction SilentlyContinue;"
    if errorlevel 1 (
        echo Failed to prepare the Instruments folder.
        exit /b 1
    )

    echo Instruments folder created successfully.
exit /b 0

:clean-all
    rd /s /q %BUILD_DIR%
exit /b
