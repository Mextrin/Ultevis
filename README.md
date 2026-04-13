# Airchestra

Airchestra is a Windows-first C++ desktop prototype for a KTH II1305 gesture-controlled music system.

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
- vcpkg manifest dependencies for OpenCV and ImGui.

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

## Prerequisites

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

If you clone this repo fresh, initialize submodules:

```powershell
git submodule update --init --recursive
```

## Build

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

## Run The App

```powershell
.\build\windows-vcpkg-debug\Airchestra.exe
```

Expected result:

- A desktop window titled `Airchestra` opens.
- The landing page shows the app name, subtitle, description, Start button, Settings button, About button, and a debug overlay toggle.
- Clicking Start opens the Control Room and marks the placeholder session as running.
- UI interactions write events to the local JSONL log.

## Smoke Tests

Run the app smoke test:

```powershell
Start-Process ".\build\windows-vcpkg-debug\Airchestra.exe" -ArgumentList "--smoke-test" -Wait
```

Run the OpenCV smoke test:

```powershell
.\build\windows-vcpkg-debug\opencv_smoke.exe
```

Optional camera smoke path:

```powershell
.\build\windows-vcpkg-debug\opencv_smoke.exe --camera
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
