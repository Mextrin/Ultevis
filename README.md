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

Clone with the JUCE submodule:

```powershell
git clone --branch airchestra-macos-support --recurse-submodules https://github.com/Mextrin/Ultevis.git
cd Ultevis
```

If the repository was cloned without submodules:

```powershell
git submodule update --init --recursive
```

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
- CMake 3.25 or newer.
- Ninja.
- vcpkg installed locally, for example at `$HOME/dev/vcpkg`.
- JUCE submodule initialized under `external/JUCE`.

Install common tools with Homebrew if needed:

```bash
brew install cmake ninja git
```

Install vcpkg if needed:

```bash
mkdir -p "$HOME/dev"
git clone https://github.com/microsoft/vcpkg.git "$HOME/dev/vcpkg"
"$HOME/dev/vcpkg/bootstrap-vcpkg.sh" -disableMetrics
export VCPKG_ROOT="$HOME/dev/vcpkg"
```

If you already have vcpkg somewhere else, set `VCPKG_ROOT` to that path instead.

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
export VCPKG_ROOT="$HOME/dev/vcpkg"
cmake --preset macos-vcpkg-debug
cmake --build --preset macos-vcpkg-debug
```

For Intel Macs:

```bash
cd Ultevis
export VCPKG_ROOT="$HOME/dev/vcpkg"
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

Run the OpenCV smoke test:

```powershell
.\build\windows-vcpkg-debug\opencv_smoke.exe
```

On macOS:

```bash
./build/macos-vcpkg-debug/opencv_smoke
```

Optional camera smoke path:

```powershell
.\build\windows-vcpkg-debug\opencv_smoke.exe --camera
```

On macOS:

```bash
./build/macos-vcpkg-debug/opencv_smoke --camera
```

## Logs

Airchestra writes local event logs to:

```text
build/windows-vcpkg-debug/logs/airchestra-events.jsonl
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
