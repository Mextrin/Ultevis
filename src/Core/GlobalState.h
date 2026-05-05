#pragma once
#include <atomic>
#include <array>

enum class ActiveInstrument {
   Theremin = 0,
   Drums = 1,
   Keyboard = 2,
   Guitar = 3
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

enum class GuitarSound {
   CleanElectric = 0,
   DistortedElectric = 1,
   Acoustic = 2
};

enum class GuitarStrumDirection {
   Down = 0,
   Up = 1
};

enum class GuitarChordRoot {
   C = 0,
   D = 1,
   E = 2,
   F = 3,
   G = 4,
   A = 5,
   B = 6
};

enum class GuitarChordQuality {
   Major = 0,
   Minor = 1,
   Dom7 = 2,
   Maj7 = 3,
   Min7 = 4,
   Sus2 = 5,
   Sus4 = 6
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

   // --- KEYBOARD STATE ---
   std::array<std::atomic<bool>, 128> keyboardState {};
   std::array<std::atomic<bool>, 128> keyboardTopLeftState {};
   std::array<std::atomic<bool>, 128> keyboardTopRightState {};
   std::array<std::atomic<bool>, 128> keyboardBottomLeftState {};
   std::array<std::atomic<bool>, 128> keyboardBottomRightState {};
   /// Last MIDI velocity (1–127) for noteOn; set from camera UDP using left/right hand master %.
   std::array<std::atomic<int>, 128> keyboardNoteVelocity {};
   std::atomic<int> leftKeyboardVelocity { 100 };   // % 0–100 (UI), same semantics as drum hands
   std::atomic<int> rightKeyboardVelocity { 100 };
   std::atomic<KeyboardSound> currentKeyboardInstrument { KeyboardSound::GrandPiano };
   std::atomic<bool> sustainPedal { false };
   std::atomic<int> topKeyboardOctave { 5 };
   std::atomic<int> bottomKeyboardOctave { 4 };
   std::atomic<bool> rightThumbUp { false };
   std::atomic<bool> rightThumbDown { false };
   std::atomic<bool> leftThumbUp { false };
   std::atomic<bool> leftThumbDown { false };

   // --- GUITAR STATE ---
   std::atomic<GuitarSound> currentGuitarSound { GuitarSound::CleanElectric };
   std::atomic<bool> guitarStrumHit { false };
   std::atomic<GuitarStrumDirection> guitarStrumDirection { GuitarStrumDirection::Down };
   std::atomic<GuitarChordRoot> currentGuitarRoot { GuitarChordRoot::C };
   std::atomic<GuitarChordQuality> currentGuitarQuality { GuitarChordQuality::Major };
   std::atomic<int> guitarVelocity { 100 };
   std::atomic<bool> guitarNeckUp   { false };
   std::atomic<bool> guitarNeckDown { false };

   // --Routing and instrument selection--
   std::atomic<bool> routeToInternalAudio { true };
   std::atomic<bool> routeToMidiOut       { true };
   std::atomic<float> masterVolume        { 1.0f }; // 0-1
   std::atomic<ActiveInstrument> currentInstrument {ActiveInstrument::Theremin};

   // -- Camera Control --
   std::atomic<bool> requestStopCameraSession{false};
   std::atomic<bool> cameraSessionActive{false};

};
