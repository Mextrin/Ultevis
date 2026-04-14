// src/main.cpp

/*
==============================================================================
MAIN ENTRY POINT (Application Bootstrap & High-Level Orchestration)

This file is responsible for bootstrapping the entire application and wiring
together the core subsystems. It initializes the shared GlobalState object,
starts the audio engine (which opens the audio device and begins real-time
processing), and optionally runs a simulation of hand movement for testing.

The main function demonstrates how gesture input (normally coming from the
camera thread) affects the system by directly manipulating GlobalState. This
includes triggering note-on/note-off events via visibility flags and modifying
pitch/volume via positional values.

After the simulation phase, control is handed over to the real camera pipeline
(startCameraFeed), which becomes the producer of hand-tracking data.

Key responsibilities:
- Construct and own the GlobalState (shared across threads)
- Initialize and start the HeadlessAudioEngine (audio thread)
- Optionally simulate gesture input for testing/debugging
- Launch the camera/vision subsystem

This file does NOT perform any audio processing or synthesis itself. It strictly
acts as the top-level coordinator of system components.
==============================================================================
*/

#include "Core/GlobalState.h"
#include "Audio/AudioEngine.h"
#include <iostream>
#include <thread>
#include <chrono>
extern void startCameraFeed();

int main() {
    std::cout << "Booting JUCE Synthesiser..." << std::endl;

    GlobalState state;
    
    // only for testing, will be replaced by ui
    char midiChoice = 'n';
    std::cout << "Enable MIDI output? (y/n): ";
    std::cin >> midiChoice;

    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        midiChoice = 'n';
    }
    //


    state.routeToMidiOut.store(midiChoice == 'y' || midiChoice == 'Y');

    HeadlessAudioEngine audio(&state); // Soundcard turns on here!

    std::cout << "Simulating right hand moving left-to-right (Pitch Up)" << std::endl;

    // 1. Tell the engine the hands are visible (triggers Note On)
    state.rightHandVisible.store(true);
    state.leftHandVisible.store(true);
    state.leftHandY.store(0.2f); // Keep volume loud and steady

    // 2. Simulate the Right Hand sliding across the X-axis over 3 seconds
    for (float x = 0.0f; x <= 1.0f; x += 0.02f) {
        state.rightHandX.store(x); 
        std::this_thread::sleep_for(std::chrono::milliseconds(60)); // Wait a tiny bit
    }

    std::cout << "Simulation complete. Stopping Note." << std::endl;

    // 3. Hide the hands (triggers Note Off)
    state.rightHandVisible.store(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Let the ADSR fade out

    startCameraFeed();

    return 0;
}
 
 
 