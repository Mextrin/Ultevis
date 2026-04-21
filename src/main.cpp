#include "Core/GlobalState.h"
#include "Audio/AudioEngine.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
extern void startCameraFeed(GlobalState* state);

int main() {
    GlobalState state;
    
    // only for testing, will be replaced by ui

    float volumeChoice = 1.0f;
    std::cout << "Set master volume (0.0 to 1.0): ";
    std::cin >> volumeChoice;
    state.masterVolume.store(volumeChoice);

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
    std::cout << "Select Instrument [0] Theremin, [1] Drums, [2] Keyboard: ";
    std::cin >> instChoice;

    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        instChoice = '0';
    }

    if (instChoice == '1') {
        state.currentInstrument.store(ActiveInstrument::Drums);
        audio.loadDrumSound("Instruments/SMDrums_Sforzando_1.2/Programs/SM_Drums_kit.sfz");
        for (int i = 0; i < 4; ++i) 
        {
            // 8th note hihat beat
            state.leftDrumType.store(36);
            state.leftDrumVelocity.store(100);
            state.leftDrumHit.store(true);
            state.rightDrumType.store(42);
            state.rightDrumVelocity.store(100);
            state.rightDrumHit.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            state.rightDrumType.store(42);
            state.rightDrumVelocity.store(100);
            state.rightDrumHit.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            state.rightDrumType.store(42);
            state.rightDrumVelocity.store(100);
            state.rightDrumHit.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            state.rightDrumType.store(42);
            state.rightDrumVelocity.store(100);
            state.rightDrumHit.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            state.rightDrumType.store(42);
            state.rightDrumVelocity.store(100);
            state.rightDrumHit.store(true); 
            state.leftDrumType.store(38);
            state.leftDrumVelocity.store(100);
            state.leftDrumHit.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            state.rightDrumType.store(42);
            state.rightDrumVelocity.store(100);
            state.rightDrumHit.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            state.rightDrumType.store(42);
            state.rightDrumVelocity.store(100);
            state.rightDrumHit.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            state.rightDrumType.store(42);
            state.rightDrumVelocity.store(100);
            state.rightDrumHit.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    } else if (instChoice == '2') {
        state.currentInstrument.store(ActiveInstrument::Keyboard);
        char keyboardSoundChoice = '0';
        std::cout << "Select Waveform [0] Grand Piano, [1] Organ, [2] Flute, [3] Harp, [4] Violin: ";
        std::cin >> keyboardSoundChoice;
        int keyboardSoundID;

        if (keyboardSoundChoice == '0') {
            state.currentKeyboardInstrument.store(KeyboardSound::GrandPiano);
            keyboardSoundID = 0;
        }
        else if (keyboardSoundChoice == '1') {
            state.currentKeyboardInstrument.store(KeyboardSound::Organ);
            keyboardSoundID = 1;
        }
        else if (keyboardSoundChoice == '2') {
            state.currentKeyboardInstrument.store(KeyboardSound::Flute);
            keyboardSoundID = 2;
        }
        else if (keyboardSoundChoice == '3') {
            state.currentKeyboardInstrument.store(KeyboardSound::Harp);
            keyboardSoundID = 3;
        }
        else if (keyboardSoundChoice == '4') {
            state.currentKeyboardInstrument.store(KeyboardSound::Violin);
            keyboardSoundID = 4;
        }

        audio.loadKeyboardSound(keyboardSoundID);
        int melodyNotes[] = { 60, 64, 67, 72 };
        for (int i = 0; i < 4; ++i) 
        {
            // 1. PRESS THE KEY DOWN
            state.keyboardNote.store(melodyNotes[i]);
            state.keyboardVelocity.store(100);
            state.isKeyPressed.store(true);
            
            // Hold the note for 400ms
            std::this_thread::sleep_for(std::chrono::milliseconds(400));

            // 2. LIFT THE FINGER UP
            state.isKeyPressed.store(false);
            
            // Wait 100ms in silence before hitting the next key
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
        }
        
        std::cout << "Keyboard test finished!" << std::endl;

    } else {
        state.currentInstrument.store(ActiveInstrument::Theremin);
        std::cout << "\n>>> THEREMIN ACTIVE <<<\n" << std::endl;

        char waveChoice = '0';
        std::cout << "Select Waveform [0] Sine, [1] Square, [2] Saw, [3] Triangle: ";
        std::cin >> waveChoice;

        if (waveChoice == '1')
            state.currentWaveform.store(Waveform::Square);
        else if (waveChoice == '2')
            state.currentWaveform.store(Waveform::Saw);
        else if (waveChoice == '3')
            state.currentWaveform.store(Waveform::Triangle);
        else
            state.currentWaveform.store(Waveform::Sine);
    }

    std::cout << "\nWaiting For Python Script To Start Camera\n" << std::endl;
    if (std::getenv("ULTEVIS_LAUNCH_HAND_DETECTOR") != nullptr) {
        const char* detectorScript = std::getenv("ULTEVIS_HAND_DETECTOR_SCRIPT");
        const std::string scriptPath = detectorScript != nullptr
            ? detectorScript
            : "src\\mediapipe\\hand_detector.py";
#ifdef _WIN32
        const std::string command = "start \"Ultevis Hand Detector\" python \"" + scriptPath + "\"";
#else
        const std::string command = "python3 \"" + scriptPath + "\" &";
#endif
        std::system(command.c_str());
    }
    startCameraFeed(&state);

    return 0;
}