// src/main.cpp
#include "Core/GlobalState.h"
#include "Audio/AudioEngine.h"
#include <iostream>
#include <thread>
#include <chrono>
extern void startCameraFeed();

int main() {
    std::cout << "Booting JUCE Synthesiser..." << std::endl;

    GlobalState state;
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
 
 
 