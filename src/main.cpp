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
extern void startCameraFeed(GlobalState* state);

int main() {
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
    
    state.routeToMidiOut.store(midiChoice == 'y' || midiChoice == 'Y');

    HeadlessAudioEngine audio(&state); // Soundcard turns on here!

    std::cout << "\nBooted! Defaulting to Theremin.\n" << std::endl;

    // INSTRUMENT SELECTION
    char instChoice = '0';
    std::cout << "Select Instrument [0] Theremin, [1] Drums: ";
    std::cin >> instChoice;

    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        instChoice = '0';
    }

    if (instChoice == '1') {
        state.currentInstrument.store(ActiveInstrument::Drums);
        for (int i = 0; i < 4; ++i) 
        {
            // KICK AND HIHAT SAME TIME
            state.leftDrumType.store(36);
            state.leftDrumVelocity.store(100);
            state.leftDrumHit.store(true);
            state.rightDrumType.store(42);
            state.rightDrumVelocity.store(100);
            state.rightDrumHit.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    } 
    else {
        state.currentInstrument.store(ActiveInstrument::Theremin);
        std::cout << "\n>>> THEREMIN ACTIVE <<<\n" << std::endl;
    }

    std::cout << "\nWaiting For Python Script To Start Camera\n" << std::endl;
    startCameraFeed(&state);

    return 0;
}
 
 
 