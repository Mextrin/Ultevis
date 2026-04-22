#include "TerminalUI.h"
#include "Core/GlobalState.h"

#include <iostream>
#include <limits>
#include <thread>
#include <chrono>

namespace
{
    // Clears bad input from std::cin to recover from invalid user input
    void clearBadInput()
    {
        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }

    // Triggers a drum hit by writing to the shared state (left or right hand)
    void triggerDrumHit(GlobalState& state, int drumType, int velocity, bool isLeftHand)
    {
        if (isLeftHand)
        {
            state.leftDrumType.store(drumType);
            state.leftDrumVelocity.store(velocity);
            state.leftDrumHit.store(true);
        }
        else
        {
            state.rightDrumType.store(drumType);
            state.rightDrumVelocity.store(velocity);
            state.rightDrumHit.store(true);
        }
    }

    // Simulates pressing a keyboard note for a fixed duration
    void pressKeyboardNote(GlobalState& state, int note, int velocity, int holdMs, int gapMs)
    {
        state.keyboardNote.store(note);
        state.keyboardVelocity.store(velocity);
        state.isKeyPressed.store(true);

        std::this_thread::sleep_for(std::chrono::milliseconds(holdMs));

        state.isKeyPressed.store(false);

        std::this_thread::sleep_for(std::chrono::milliseconds(gapMs));
    }
}

namespace TerminalUI
{
    // === INPUT PROMPTS ===

    float askMasterVolume()
    {
        float volumeChoice = 1.0f;
        std::cout << "Set master volume (0.0 to 1.0): ";
        std::cin >> volumeChoice;

        if (std::cin.fail())
        {
            clearBadInput();
            return 1.0f;
        }

        if (volumeChoice < 0.0f) volumeChoice = 0.0f;
        if (volumeChoice > 1.0f) volumeChoice = 1.0f;

        return volumeChoice;
    }

    bool askEnableMidi()
    {
        char midiChoice = 'n';
        std::cout << "Enable MIDI output? (y/n): ";
        std::cin >> midiChoice;

        if (std::cin.fail())
        {
            clearBadInput();
            return false;
        }

        return midiChoice == 'y' || midiChoice == 'Y';
    }

    char askInstrumentChoice()
    {
        char instChoice = '0';

        std::cout << "\nSelect Instrument:\n"
                  << "[0] Theremin\n"
                  << "[1] Drums\n"
                  << "[2] Keyboard\n"
                  << "[q] Quit\n> ";

        std::cin >> instChoice;

        if (std::cin.fail())
        {
            clearBadInput();
            return '0';
        }

        return instChoice;
    }

    char askThereminWaveChoice()
    {
        char waveChoice = '0';

        std::cout << "Select Waveform:\n"
                  << "[0] Sine\n"
                  << "[1] Square\n"
                  << "[2] Saw\n"
                  << "[3] Triangle\n> ";

        std::cin >> waveChoice;

        if (std::cin.fail())
        {
            clearBadInput();
            return '0';
        }

        return waveChoice;
    }

    char askKeyboardSoundChoice()
    {
        char choice = '0';

        std::cout << "Select Keyboard Sound:\n"
                  << "[0] Grand Piano\n"
                  << "[1] Organ\n"
                  << "[2] Flute\n"
                  << "[3] Harp\n"
                  << "[4] Violin\n> ";

        std::cin >> choice;

        if (std::cin.fail())
        {
            clearBadInput();
            return '0';
        }

        return choice;
    }


    // === CONFIGURATION ===

    void configureThereminWaveform(GlobalState& state, char waveChoice)
    {
        switch (waveChoice)
        {
            case '1':
                state.currentWaveform.store(Waveform::Square);
                break;
            case '2':
                state.currentWaveform.store(Waveform::Saw);
                break;
            case '3':
                state.currentWaveform.store(Waveform::Triangle);
                break;
            case '0':
            default:
                state.currentWaveform.store(Waveform::Sine);
                break;
        }
    }

    int configureKeyboardSound(GlobalState& state, char choice)
    {
        switch (choice)
        {
            case '1':
                state.currentKeyboardInstrument.store(KeyboardSound::Organ);
                return 1;
            case '2':
                state.currentKeyboardInstrument.store(KeyboardSound::Flute);
                return 2;
            case '3':
                state.currentKeyboardInstrument.store(KeyboardSound::Harp);
                return 3;
            case '4':
                state.currentKeyboardInstrument.store(KeyboardSound::Violin);
                return 4;
            case '0':
            default:
                state.currentKeyboardInstrument.store(KeyboardSound::GrandPiano);
                return 0;
        }
    }


    // === TEST / DEMO ===

    void playDrumTest(GlobalState& state)
    {
        std::cout << "\nPlaying drum test...\n";

        triggerDrumHit(state, 36, 100, true);   // kick
        triggerDrumHit(state, 42, 100, false);  // hi-hat
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        triggerDrumHit(state, 38, 100, true);   // snare
        triggerDrumHit(state, 42, 100, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        std::cout << "Drum test finished!\n";
    }

    void playKeyboardTest(GlobalState& state)
    {
        std::cout << "\nPlaying keyboard test...\n";

        const int melodyNotes[] = {60, 64, 67, 72};

        for (int note : melodyNotes)
        {
            pressKeyboardNote(state, note, 100, 400, 100);
        }

        std::cout << "Keyboard test finished!\n";
    }
}