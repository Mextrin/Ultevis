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
