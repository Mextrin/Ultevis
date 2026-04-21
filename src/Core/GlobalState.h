#pragma once
#include <atomic>

enum class ActiveInstrument {
    Theremin = 0,
    Drums    = 1,
    Keyboard = 2
};

enum class Waveform {
    Sine     = 0,
    Square   = 1,
    Saw      = 2,
    Triangle = 3
};

enum class KeyboardSound {
    GrandPiano = 0,
    Organ      = 1,
    Flute      = 2,
    Harp       = 3,
    Violin     = 4
};

class GlobalState {
public:
    // --- Theremin continuous controls ---
    std::atomic<float> rightHandX        { 0.5f };   // 0–1, maps to pitch
    std::atomic<float> leftHandY         { 1.0f };   // 0–1, maps to volume
    std::atomic<bool>  rightHandVisible  { false };
    std::atomic<bool>  leftHandVisible   { false };
    std::atomic<Waveform> currentWaveform { Waveform::Sine };

    // --- Theremin synthesis range ---
    std::atomic<float> freqMin     { 220.0f };  // Hz, left edge of hand range
    std::atomic<float> freqMax     { 880.0f };  // Hz, right edge of hand range
    std::atomic<float> volumeFloor { 0.2f  };   // minimum volume when left hand is visible

    // --- Drums ---
    std::atomic<bool> leftDrumHit      { false };
    std::atomic<int>  leftDrumType     { 36 };
    std::atomic<int>  leftDrumVelocity { 100 };
    std::atomic<bool> rightDrumHit      { false };
    std::atomic<int>  rightDrumType     { 38 };
    std::atomic<int>  rightDrumVelocity { 100 };

    // --- Keyboard ---
    std::atomic<bool> isKeyPressed   { false };
    std::atomic<int>  keyboardNote   { 60 };
    std::atomic<int>  keyboardVelocity { 100 };
    std::atomic<KeyboardSound> currentKeyboardInstrument { KeyboardSound::GrandPiano };
    std::atomic<bool> sustainPedal   { false };

    // --- Routing & global gain ---
    std::atomic<bool>  routeToInternalAudio { true };
    std::atomic<bool>  routeToMidiOut       { false };
    std::atomic<float> masterVolume         { 1.0f };  // 0–1
    std::atomic<ActiveInstrument> currentInstrument { ActiveInstrument::Theremin };
};
