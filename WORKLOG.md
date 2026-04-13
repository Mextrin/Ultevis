# Airchestra Worklog

## 2026-04-13

### Milestone 1 - Build wiring

- Status: Complete.
- Implemented: Created `PLAN.md`; added `imgui[opengl3-binding]` to `vcpkg.json`; replaced the GUI target with `Airchestra`; linked JUCE GUI/OpenGL, OpenGL, and ImGui.
- Commands: `cmake --preset windows-vcpkg-debug`
- Result: Configure succeeded; vcpkg installed Dear ImGui 1.92.7 for `x64-windows`.
- Known limitations: Runtime UI validation still pending.

### Milestone 2 - Bootstrap, ImGui UI, and logging code

- Status: Complete.
- Implemented: Added app/window bootstrap, event logger, view state, ImGui/OpenGL layer, landing page, debug overlay, and UI renderer under `src/airchestra`.
- Commands: `cmake --build --preset windows-vcpkg-debug --target Airchestra`
- Result: Build succeeded and produced `build/windows-vcpkg-debug/Airchestra.exe`.
- Known limitations: Interactive visual validation still pending; smoke test pending.

### Milestone 3 - Smoke validation

- Status: Complete.
- Implemented: Added `--smoke-test` mode to exercise app startup and logging without leaving the GUI open.
- Commands: `Start-Process ".\build\windows-vcpkg-debug\Airchestra.exe" -ArgumentList "--smoke-test" -Wait`; `.\build\windows-vcpkg-debug\opencv_smoke.exe`; interactive launch/close check for `Airchestra.exe`.
- Result: Airchestra smoke exited with code `0`; OpenCV smoke exited with code `0`; interactive launch reported main window title `Airchestra` and closed cleanly; log contains `app_started`, `main_window_created`, `imgui_initialized`, `landing_page_shown`, and `app_closing`.
- Known limitations: Camera, audio, MIDI, and MediaPipe integration remain intentionally out of scope for this UI/debug skeleton.

### Milestone 4 - Clickable UX state

- Status: Complete.
- Implemented: Added real app navigation state for Landing, Control Room, Settings, and About; made Start/Stop session, overlay controls, settings checkboxes, mock sensitivity, panel cards, and log-folder reveal actions clickable; expanded the event vocabulary for `screen_changed`, `panel_selected`, `setting_changed`, and `session_state_changed`.
- Commands: `cmake --build --preset windows-vcpkg-debug --target Airchestra`; `Start-Process ".\build\windows-vcpkg-debug\Airchestra.exe" -ArgumentList "--smoke-test" -Wait`; interactive launch/close check for `Airchestra.exe`.
- Result: Build succeeded; smoke exited with code `0` and logged the new interaction events; interactive launch reported main window title `Airchestra` and closed cleanly.
- Known limitations: Controls are useful UI/debug state only; they still do not initialize real camera, audio, MIDI, or MediaPipe systems.

### Milestone 5 - macOS build branch

- Status: Implemented and pushed from Windows; macOS runtime validation still requires a Mac.
- Implemented: Added macOS CMake presets for Apple Silicon and Intel builds; made vcpkg OpenCV features platform-specific; added macOS bundle metadata; changed the OpenCV smoke camera backend to use AVFoundation on macOS; documented macOS clone/build/run commands in `README.md`.
- Commands: `cmake --list-presets`; `cmake --build --preset windows-vcpkg-debug`; `Start-Process ".\build\windows-vcpkg-debug\Airchestra.exe" -ArgumentList "--smoke-test" -Wait`; `.\build\windows-vcpkg-debug\opencv_smoke.exe`.
- Result: Windows configure/build and smoke tests still pass after the cross-platform changes.
- Known limitations: The macOS presets were not executed locally because this work was done on Windows.

### Milestone 6 - Active Airchestra theremin integration

- Status: Complete on Windows; macOS compile validation still requires a Mac.
- Implemented: Linked the reusable theremin engine files into the active `Airchestra` target; added JUCE audio/MIDI modules; made `MainComponent` own the shared hand-control state, mock input, sine audio output, and MIDI output; added live x/y, frequency, pitch bend, CC11, audio, and MIDI readouts to the ImGui Control Room and overlay; added `midi_status_changed` logging.
- Commands: `cmake --preset windows-vcpkg-debug`; `cmake --build --preset windows-vcpkg-debug --target Airchestra`; `Start-Process ".\build\windows-vcpkg-debug\Airchestra.exe" -ArgumentList "--smoke-test" -Wait`; `.\build\windows-vcpkg-debug\opencv_smoke.exe`; interactive launch/close check for `Airchestra.exe`.
- Result: Windows configure/build succeeded; Airchestra smoke exited successfully; OpenCV smoke exited successfully; interactive launch reported main window title `Airchestra` and closed cleanly.
- Known limitations: Real camera/MediaPipe input is still not connected; audio audibility and external DAW MIDI reception still need a manual session check with speakers and loopMIDI/Ableton.

### Milestone 7 - Control Room launch reliability

- Status: Complete on Windows.
- Implemented: Fixed the top tab navigation so the app no longer bounces back to `Home` when the Control Room opens; added `--autostart-session` so the Windows theremin prototype can launch directly into a running Control Room session; updated `README.md` with the verified Windows theremin launch path.
- Commands: `cmake --build --preset windows-vcpkg-debug --target Airchestra`; `.\build\windows-vcpkg-debug\Airchestra.exe --autostart-session`; screenshot check of the Control Room plus log verification for `session_state_changed`.
- Result: The app now opens to the Control Room with changing x/y/frequency values visible while the sine theremin is running; the session no longer jumps back to the landing page.
- Known limitations: Manual speaker output and external DAW MIDI reception still depend on the local Windows audio/MIDI environment.
