#pragma once
#include <atomic>

class GlobalState {
public:
    // ==============================================================================
    // 1. SPATIAL DATA
    // ==============================================================================

    // Right hand controls Pitch (X-axis: 0.0 is far left, 1.0 is far right)
    std::atomic<float> rightHandX { 0.5f }; 

    // Left hand controls Volume (Y-axis: 0.0 is top, 1.0 is bottom)
    // Defaults to 1.0f so the volume starts at zero (1.0 - 1.0 = 0.0)
    std::atomic<float> leftHandY { 1.0f }; 


    // ==============================================================================
    // 2. TRIGGERS
    // ==============================================================================

    // Tells the Audio Engine to trigger the Note On/Off (ADSR envelope)
    std::atomic<bool> rightHandVisible { false };

    // Tells the Audio Engine to dynamically mute the volume if the hand drops out of frame
    std::atomic<bool> leftHandVisible { false };


    // ==============================================================================
    // 3. ROUTING STATE (For UI Toggles)
    // ==============================================================================

    // Determines if the JUCE Synthesiser actually makes sound
    std::atomic<bool> routeToInternalAudio { true };

    // Determines if the UI Thread should broadcast MIDI to Ableton
    std::atomic<bool> routeToMidiOut { false };
};

 
 
 