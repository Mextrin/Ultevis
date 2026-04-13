# Airchestra ImGui UI + Logging Skeleton Plan

## Goal

Create a Windows-first C++/CMake/JUCE desktop skeleton for Airchestra that opens a main window, initializes Dear ImGui, renders a landing page and debug overlay, and records key app/UI events to a lightweight JSONL log.

## Milestones

1. **Build wiring**
   - Add Dear ImGui through vcpkg with the OpenGL3 backend.
   - Keep the existing OpenCV smoke target.
   - Replace the current JUCE GUI app target with a new `Airchestra` target and link JUCE GUI/OpenGL plus ImGui.

2. **Bootstrap and logging**
   - Add `EventLogger` with a small event vocabulary and JSONL output at `logs/airchestra-events.jsonl` next to the executable.
   - Add `ViewState` for UI status and overlay visibility.
   - Add `Main.cpp` and `MainComponent` for the `Airchestra` app/window lifecycle.

3. **ImGui rendering**
   - Add `ImGuiLayer` as a JUCE `Component` and `juce::OpenGLRenderer`.
   - Initialize/shutdown ImGui and the OpenGL3 backend cleanly.
   - Feed basic mouse input from JUCE to ImGui.

4. **UI views**
   - Add `LandingPageView`, `OverlayView`, and `UiRenderer`.
   - Render the landing page controls and the debug overlay placeholders for Camera, Audio, UI, System State, Interaction Log, and App status.
   - Log button clicks, overlay toggles, landing page shown, and ImGui initialization.

5. **Validation**
   - Configure and build with `cmake --preset windows-vcpkg-debug` and `cmake --build --preset windows-vcpkg-debug`.
   - Run `Airchestra.exe --smoke-test` and the existing `opencv_smoke.exe`.
   - Document status, commands, and limitations in `WORKLOG.md`.

## Out Of Scope

- Camera capture, MediaPipe integration, audio synthesis, MIDI routing, and Ableton integration.
- Heavy UI framework changes or Qt.
- Logging secrets or user-sensitive data.
