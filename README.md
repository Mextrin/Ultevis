# Airchestra

Airchestra is a cross-platform C++ desktop prototype for a KTH II1305 gesture-controlled music system. The project is Windows-first for the II1305 team workflow, but this branch also includes macOS build presets for Apple Silicon and Intel Macs.

The current repository contains the Week 1 application scaffold: CMake, vcpkg dependencies, a JUCE desktop window, a Dear ImGui UI/debug layer, OpenCV verification, and lightweight local event logging. Camera capture, MediaPipe landmarks, real audio synthesis, and MIDI output are intentionally not wired into the active app target yet.

## What Is Included

- `Airchestra`: the main JUCE desktop app target.
- `opencv_smoke`: a tiny OpenCV C++ smoke-test executable.
- Dear ImGui integration through JUCE OpenGL.
- A polished first-run landing page with Start, Settings, and About controls.
- A clickable Control Room with placeholder panels for Camera, Audio, UI, System State, and Interaction Log.
- A debug overlay that can be toggled from the UI.
- Structured JSONL event logging for app startup, window creation, ImGui initialization, screen changes, panel selection, button clicks, settings changes, session state changes, overlay toggles, and app shutdown.
- JUCE as a Git submodule under `external/JUCE`.
- vcpkg manifest dependencies for OpenCV and ImGui, with platform-specific OpenCV camera backend features.

## Repository Layout

```text
.
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
├── vcpkg-configuration.json
├── external/
│   └── JUCE/
├── src/
│   ├── opencv_smoke.cpp
│   ├── airchestra/
│   │   ├── EventLogger.*
│   │   ├── ImGuiLayer.*
│   │   ├── LandingPageView.*
│   │   ├── Main.*
│   │   ├── MainComponent.*
│   │   ├── OverlayView.*
│   │   ├── UiActions.*
│   │   ├── UiRenderer.*
│   │   └── ViewState.h
│   └── theremin/
└── WORKLOG.md
```

`src/theremin` contains earlier Week 1 theremin/audio/MIDI prototype code, but it is not currently linked into the active `Airchestra` target. The active UI/debug skeleton lives in `src/airchestra`.

## Clone

Always use the `airchestra-macos-support` branch for the macOS flow in this repository. Do not switch to `main`.

Clone with the JUCE submodule:

```bash
git clone --branch airchestra-macos-support --recurse-submodules https://github.com/Mextrin/Ultevis.git
cd Ultevis
```

If the repository was cloned without submodules:

```bash
git submodule update --init --recursive
```

## Start Here On A New Mac

These are the exact steps to go from a fresh macOS machine to a running Airchestra app from the `airchestra-macos-support` branch. This flow was verified on macOS Tahoe 26.3 on Apple Silicon and also includes the Intel path.

### 1. Install Xcode Command Line Tools

Run:

```bash
xcode-select -p >/dev/null 2>&1 || xcode-select --install
```

If macOS opens an install dialog, finish that install first, then come back to Terminal and continue.

### 2. Install Homebrew If Needed

Check whether Homebrew is already installed:

```bash
command -v brew
```

If that prints nothing, install Homebrew:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Then load Homebrew into the current shell:

```bash
if [ -x /opt/homebrew/bin/brew ]; then
  eval "$(/opt/homebrew/bin/brew shellenv)"
elif [ -x /usr/local/bin/brew ]; then
  eval "$(/usr/local/bin/brew shellenv)"
fi
```

### 3. Install Build Tools

Install the tools required by the macOS presets:

```bash
brew install cmake ninja pkgconf gh
```

`pkgconf` provides the `pkg-config` binary that the vcpkg/OpenCV configure step needs.

### 4. Make Sure GitHub Access Works

This repository may require GitHub access. If `git clone` returns a 404 or asks for credentials, sign in first:

```bash
gh auth login --web --git-protocol https
gh auth status
```

### 5. Install vcpkg

```bash
mkdir -p "$HOME/dev"
if [ ! -d "$HOME/dev/vcpkg/.git" ]; then
  git clone https://github.com/microsoft/vcpkg.git "$HOME/dev/vcpkg"
fi
"$HOME/dev/vcpkg/bootstrap-vcpkg.sh" -disableMetrics
export VCPKG_ROOT="$HOME/dev/vcpkg"
```

### 6. Clone This Branch

```bash
mkdir -p "$HOME/dev"
cd "$HOME/dev"
git clone --branch airchestra-macos-support --recurse-submodules https://github.com/Mextrin/Ultevis.git
cd Ultevis
```

If you already have the repository locally, update it instead:

```bash
cd "$HOME/dev/Ultevis"
git fetch origin
git switch airchestra-macos-support
git pull --ff-only
git submodule update --init --recursive
```

### 7. Verify The Checkout

```bash
git status --short --branch
git log --oneline --decorate -3
git submodule status
git -C external/JUCE describe --tags --always --dirty
```

You should be on `airchestra-macos-support`. For the macOS OpenGL startup fix, make sure your branch includes commit `d7d5e96` or anything newer.

### 8. Verify Prerequisites

```bash
xcode-select -p
cmake --version
ninja --version
git --version
echo "$VCPKG_ROOT"
uname -m
```

### 9. Build Airchestra

Run this exact block from the repository root:

```bash
export VCPKG_ROOT="${VCPKG_ROOT:-$HOME/dev/vcpkg}"
cmake --preset macos-qt-debug
cmake --build --preset macos-qt-debug
open build/macos-qt-debug/Airchestra.app/
```

Build output locations:

- Apple Silicon: `build/macos-vcpkg-debug/`
- Intel: `build/macos-vcpkg-debug-x64/`

### 10. Run Smoke Tests

```bash
ARCH="$(uname -m)"

if [ "$ARCH" = "arm64" ]; then
  ./build/macos-vcpkg-debug/Airchestra.app/Contents/MacOS/Airchestra --smoke-test
  ./build/macos-vcpkg-debug/opencv_smoke
else
  ./build/macos-vcpkg-debug-x64/Airchestra.app/Contents/MacOS/Airchestra --smoke-test
  ./build/macos-vcpkg-debug-x64/opencv_smoke
fi
```

Expected result:

- `Airchestra --smoke-test` exits successfully.
- `opencv_smoke` exits successfully and prints the OpenCV version.

Optional camera check:

```bash
ARCH="$(uname -m)"

if [ "$ARCH" = "arm64" ]; then
  ./build/macos-vcpkg-debug/opencv_smoke --camera
else
  ./build/macos-vcpkg-debug-x64/opencv_smoke --camera
fi
```

### 11. Launch The App

```bash
find build -name "Airchestra.app" -print
```

Then open the app bundle that matches your architecture:

```bash
ARCH="$(uname -m)"

if [ "$ARCH" = "arm64" ]; then
  open build/macos-vcpkg-debug/Airchestra.app
else
  open build/macos-vcpkg-debug-x64/Airchestra.app
fi
```

Expected result:

- A desktop window titled `Airchestra` opens.
- The landing page appears.
- `Start` opens the Control Room.
- `Settings` and `About` are clickable.
- The debug overlay can be shown and hidden.
- Closing the window exits cleanly.

### 12. Check The Log File

```bash
LOG_FILE="$(find build -name 'airchestra-events.jsonl' -print | head -n 1)"
echo "$LOG_FILE"
tail -n 20 "$LOG_FILE"
```

The log should contain startup and UI events such as `app_started`, `main_window_created`, `imgui_initialized`, `landing_page_shown`, and `screen_changed`.

### macOS Troubleshooting

- If `git clone` fails with a GitHub authentication error or HTTP 404, run `gh auth login --web --git-protocol https` and retry.
- If CMake says `Could not find pkg-config`, run `brew install pkgconf`.
- If the app aborts at launch with a GLSL or OpenGL version error such as `#version 130 is not supported`, update to commit `d7d5e96` or newer on `airchestra-macos-support`.
- If you are not sure where the app bundle landed, run `find build -name "Airchestra.app" -print`.
- If `VCPKG_ROOT` is empty, run `export VCPKG_ROOT="$HOME/dev/vcpkg"` before configuring.

## Windows Prerequisites

Use Windows 11 with:

- Visual Studio 2026 Community with the C++ desktop workload.
- CMake 3.25 or newer.
- Ninja.
- vcpkg installed at `C:\Users\Tony\dev\vcpkg`.
- JUCE submodule initialized under `external/JUCE`.

Open a Developer PowerShell for Visual Studio, or load the developer environment manually:

```powershell
cmd /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"" -arch=amd64 -host_arch=amd64 && powershell"
```

Set the vcpkg root for the current shell:

```powershell
$env:VCPKG_ROOT = "C:\Users\Tony\dev\vcpkg"
```

## macOS Prerequisites

Use macOS 12 or newer with:

- Xcode Command Line Tools.
- Homebrew.
- CMake 3.25 or newer.
- Ninja.
- `pkg-config` available on `PATH` (for example via Homebrew `pkgconf`).
- vcpkg installed locally, for example at `$HOME/dev/vcpkg`.
- GitHub access to `https://github.com/Mextrin/Ultevis.git` if the repository is private.
- JUCE submodule initialized under `external/JUCE`.

For a brand-new machine, follow the exact commands in [Start Here On A New Mac](#start-here-on-a-new-mac).

## Build On Windows

From the repository root:

```powershell
cd "C:\Users\Tony\Downloads\ICT Project"
$env:VCPKG_ROOT = "C:\Users\Tony\dev\vcpkg"
cmake --preset windows-vcpkg-debug
cmake --build --preset windows-vcpkg-debug
```

The build output goes to:

```text
build/windows-vcpkg-debug/
```

## Build On macOS

For Apple Silicon Macs:

```bash
cd Ultevis
export VCPKG_ROOT="${VCPKG_ROOT:-$HOME/dev/vcpkg}"
cmake --preset macos-vcpkg-debug
cmake --build --preset macos-vcpkg-debug
```

For Intel Macs:

```bash
cd Ultevis
export VCPKG_ROOT="${VCPKG_ROOT:-$HOME/dev/vcpkg}"
cmake --preset macos-vcpkg-debug-x64
cmake --build --preset macos-vcpkg-debug-x64
```

The Apple Silicon build output goes to:

```text
build/macos-vcpkg-debug/
```

The Intel build output goes to:

```text
build/macos-vcpkg-debug-x64/
```

## Run The App On Windows

```powershell
.\build\windows-vcpkg-debug\Airchestra.exe
```

## Run The App On macOS

For Apple Silicon:

```bash
open build/macos-vcpkg-debug/Airchestra.app
```

For Intel:

```bash
open build/macos-vcpkg-debug-x64/Airchestra.app
```

If CMake places the app bundle in a generator-specific subfolder, locate it with:

```bash
find build -name "Airchestra.app" -print
```

Expected result:

- A desktop window titled `Airchestra` opens.
- The landing page shows the app name, subtitle, description, Start button, Settings button, About button, and a debug overlay toggle.
- Clicking Start opens the Control Room and marks the placeholder session as running.
- UI interactions write events to the local JSONL log.

## Smoke Tests

Run the app smoke test on Windows:

```powershell
Start-Process ".\build\windows-vcpkg-debug\Airchestra.exe" -ArgumentList "--smoke-test" -Wait
```

Run the app smoke test on macOS:

```bash
./build/macos-vcpkg-debug/Airchestra.app/Contents/MacOS/Airchestra --smoke-test
```

On Intel Macs:

```bash
./build/macos-vcpkg-debug-x64/Airchestra.app/Contents/MacOS/Airchestra --smoke-test
```

Run the OpenCV smoke test:

```powershell
.\build\windows-vcpkg-debug\opencv_smoke.exe
```

On macOS:

```bash
./build/macos-vcpkg-debug/opencv_smoke
```

On Intel Macs:

```bash
./build/macos-vcpkg-debug-x64/opencv_smoke
```

Optional camera smoke path:

```powershell
.\build\windows-vcpkg-debug\opencv_smoke.exe --camera
```

On macOS:

```bash
./build/macos-vcpkg-debug/opencv_smoke --camera
```

On Intel Macs:

```bash
./build/macos-vcpkg-debug-x64/opencv_smoke --camera
```

## Logs

Airchestra writes local event logs to:

```text
build/windows-vcpkg-debug/logs/airchestra-events.jsonl
```

On Apple Silicon macOS builds the log file is typically here:

```text
build/macos-vcpkg-debug/Airchestra.app/Contents/MacOS/logs/airchestra-events.jsonl
```

On Intel macOS builds the log file is typically here:

```text
build/macos-vcpkg-debug-x64/Airchestra.app/Contents/MacOS/logs/airchestra-events.jsonl
```

The format is line-delimited JSON. Example event names include:

- `app_started`
- `main_window_created`
- `imgui_initialized`
- `landing_page_shown`
- `button_clicked`
- `screen_changed`
- `panel_selected`
- `setting_changed`
- `session_state_changed`
- `overlay_toggled`
- `app_closing`

The logger is intentionally small and local. It does not store secrets and does not require a database or backend service.

## Current Architecture

The active app is split into small responsibilities:

- `Main.cpp`: JUCE application entry point, window creation, smoke-test mode, app lifecycle logging.
- `MainComponent`: owns app UI state, renderer, and ImGui layer.
- `ImGuiLayer`: JUCE OpenGL bridge for Dear ImGui, input forwarding, theme setup, and frame rendering.
- `ViewState`: simple in-memory state for current screen, selected panel, overlay state, session state, settings, and status text.
- `UiRenderer`: top-level UI composition and page routing.
- `LandingPageView`: first-run landing/home screen.
- `OverlayView`: debug overlay for development status and local logging information.
- `UiActions`: shared helpers for state transitions and logging UI interactions.
- `EventLogger`: thread-safe local JSONL event logger.

## What Is Not Implemented Yet

- Real webcam capture in the main app.
- MediaPipe hand landmark integration.
- OpenCV camera frames displayed in the UI.
- Real audio output from the active `Airchestra` target.
- Real MIDI output from the active `Airchestra` target.
- DAW/Ableton integration.
- Packaging/installer flow.

The next integration step is to connect a real hand-tracking producer to a shared control state, then expose those values in the Control Room before routing them into audio and MIDI modules.
