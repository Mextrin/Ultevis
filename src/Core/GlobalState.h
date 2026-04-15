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

enum class ActiveInstrument {
   Theremin = 0,
   Drums = 1
};

class GlobalState {
public:
   // --Theremin controls--
   std::atomic<float> rightHandX { 0.5f }; // 0-1
   std::atomic<float> leftHandY  { 1.0f }; // 0-1
   std::atomic<bool> rightHandVisible { false };
   std::atomic<bool> leftHandVisible  { false };

   // --Drum controls--
   std::atomic<bool> isDrumHit { false };
   std::atomic<int>   drumType     { 36 };   // 36, 38
   std::atomic<float> drumVelocity { 1.0f }; // 0-1

   // --Routing and instrument selection--
   std::atomic<bool> routeToInternalAudio { true };
   std::atomic<bool> routeToMidiOut       { true };
   std::atomic<ActiveInstrument> currentInstrument {ActiveInstrument::Theremin};
};

 
 
 