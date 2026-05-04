/*
==============================================================================
GLOBAL STATE (Thread-Safe Shared Control Data)

Lightweight lock-free container for communication between vision input,
audio engine, and UI. All members are std::atomic for safe concurrent access.

State groups:

1. Continuous Control (Theremin):
   - rightHandX → pitch
   - leftHandY  → volume
   - hand visibility → note gating

2. Trigger Events (Drums):
   - left/right drum hit flags
   - drum type (MIDI note) and velocity

3. Instrument & Routing:
   - currentInstrument (Theremin / Drums)
   - routeToInternalAudio / routeToMidiOut

4. Synthesis Parameters:
   - currentWaveform (oscillator shape)

Design:
- No locks, no blocking, no dynamic allocation
- Written by input thread, read by audio thread
- Strictly a data container (no logic)
==============================================================================
*/

#pragma once
#include <atomic>
#include <array>

enum class ActiveInstrument {
   Theremin = 0,
   Drums = 1,
   Keyboard = 2
};

enum class Waveform {
   Sine = 0,
   Square = 1,
   Saw = 2,
   Triangle = 3
};

enum class KeyboardSound {
   GrandPiano = 0,
   Organ = 1,
   Flute = 2,
   Harp = 3,
   Violin = 4
};

class GlobalState {
public:
   // --Theremin controls--
   std::atomic<float> rightHandX { 0.5f }; // 0-1
   std::atomic<float> rightHandY { 0.5f }; // 0-1
   std::atomic<float> leftHandX  { 0.5f }; // 0-1
   std::atomic<float> leftHandY  { 1.0f }; // 0-1
   std::atomic<bool> rightHandVisible { false };
   std::atomic<bool> leftHandVisible  { false };
   std::atomic<bool> rightPinch { false };
   std::atomic<bool> leftPinch  { false };
   std::atomic<Waveform> currentWaveform { Waveform::Sine };
   std::atomic<int> thereminSemitoneRangeOneSide { 24 }; //one octave up, one octave down
   std::atomic<int> thereminCenterNote { 60 }; // 60 = C4
   std::atomic<float> thereminVolumeFloor { 0.05f };  // minimum volume when left hand is at bottom

   // --LEFT HAND DRUM--
   std::atomic<bool> leftDrumHit { false }; 
   std::atomic<int> leftDrumType { 36 }; 
   std::atomic<int> leftDrumVelocity { 100 };       // velocity % 0–100 (UI → MIDI 0–127 per hit)

   // --RIGHT HAND DRUM--
   std::atomic<bool> rightDrumHit { false }; 
   std::atomic<int> rightDrumType { 38 }; 
   std::atomic<int> rightDrumVelocity { 100 };      // velocity % 0–100 (UI)
   std::atomic<bool> mouthKickHit { false };

   // --DRUM STATES--
   std::atomic<bool> mouthKickEnable { false }; 
   std::atomic<bool> snareLeverDown { false }; //false = normal snare

   // --- KEYBOARD STATE ---
   std::array<std::atomic<bool>, 128> keyboardState {};
   std::atomic<KeyboardSound> currentKeyboardInstrument { KeyboardSound::GrandPiano };
   std::atomic<bool> sustainPedal { false };
   std::atomic<int> topKeyboardOctave { 5 };
   std::atomic<int> bottomKeyboardOctave { 4 };
   std::atomic<bool> rightThumbUp { false };
   std::atomic<bool> rightThumbDown { false };
   std::atomic<bool> leftThumbUp { false };
   std::atomic<bool> leftThumbDown { false };

   // --Routing and instrument selection--
   std::atomic<bool> routeToInternalAudio { true };
   std::atomic<bool> routeToMidiOut       { true };
   std::atomic<float> masterVolume        { 1.0f }; // 0-1
   std::atomic<ActiveInstrument> currentInstrument {ActiveInstrument::Theremin};

   // -- Camera Control --
   std::atomic<bool> requestStopCameraSession{false};
   std::atomic<bool> cameraSessionActive{false};

};
