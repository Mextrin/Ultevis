/*
==============================================================================
GLOBAL STATE (Thread-Safe Shared Control Data)

This file defines a lightweight, lock-free data structure used to communicate
between the independent subsystems (camera/vision thread, audio engine,
and potentially UI). It uses std::atomic variables to ensure safe concurrent
access without introducing blocking or synchronization overhead.

The state is divided into three conceptual groups:

1. Spatial Data:
   Continuous normalized values representing hand positions.
   - rightHandX: controls pitch (horizontal axis)
   - leftHandY: controls volume (vertical axis)

2. Trigger Flags:
   Boolean signals that control note lifecycle and muting behavior.
   - rightHandVisible: drives note on/off events
   - leftHandVisible: allows dynamic muting when volume hand is lost

3. Routing State:
   Toggles that determine where output is sent.
   - routeToInternalAudio: enables/disables internal synthesis
   - routeToMidiOut: enables MIDI output to external software

Design purpose:
- Decouple input (camera), processing (audio), and output (sound/MIDI)
- Provide a minimal, real-time-safe communication layer
- Avoid locks, queues, or complex synchronization mechanisms

This object is shared across threads, with writers typically being the vision
system and readers being the audio engine.

No logic should be placed here—this is strictly a data container.
==============================================================================
*/

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
    std::atomic<bool> routeToMidiOut { true };
};

 
 
 