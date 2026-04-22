#include "Core/GlobalState.h"
#include "Audio/AudioEngine.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <limits>

extern void startCameraFeed(GlobalState* state);

namespace
{
    // Clears the input stream after invalid user input.
    void clearBadInput()
    {
        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }

    // Asks the user to set the master volume and clamps it between 0.0 and 1.0.
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

    // Asks the user whether MIDI output should be enabled.
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

    // Asks the user which instrument to use.
    char askInstrumentChoice()
    {
        char instChoice = '0';
        std::cout << "Select Instrument [0] Theremin, [1] Drums, [2] Keyboard: ";
        std::cin >> instChoice;

        if (std::cin.fail())
        {
            clearBadInput();
            return '0';
        }

        return instChoice;
    }

    // Asks the user which theremin waveform to use.
    char askThereminWaveChoice()
    {
        char waveChoice = '0';
        std::cout << "Select Waveform [0] Sine, [1] Square, [2] Saw, [3] Triangle: ";
        std::cin >> waveChoice;

        if (std::cin.fail())
        {
            clearBadInput();
            return '0';
        }

        return waveChoice;
    }

    // Asks the user which keyboard sound to use.
    char askKeyboardSoundChoice()
    {
        char choice = '0';
        std::cout << "Select Waveform [0] Grand Piano, [1] Organ, [2] Flute, [3] Harp, [4] Violin: ";
        std::cin >> choice;

        if (std::cin.fail())
        {
            clearBadInput();
            return '0';
        }

        return choice;
    }

    // Triggers a drum hit for either the left or right hand.
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

    // Plays a short built-in drum test pattern.
    void playDrumTest(GlobalState& state)
    {
        triggerDrumHit(state, 36, 100, true);   // kick
        triggerDrumHit(state, 42, 100, false);  // hihat
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        triggerDrumHit(state, 38, 100, true);   // snare
        triggerDrumHit(state, 42, 100, false);  // hihat
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        std::cout << "Drum test finished!" << std::endl;
    }

    // Simulates pressing and releasing a keyboard note.
    void pressKeyboardNote(GlobalState& state, int note, int velocity, int holdMs, int gapMs)
    {
        state.keyboardNote.store(note);
        state.keyboardVelocity.store(velocity);
        state.isKeyPressed.store(true);

        std::this_thread::sleep_for(std::chrono::milliseconds(holdMs));

        state.isKeyPressed.store(false);

        std::this_thread::sleep_for(std::chrono::milliseconds(gapMs));
    }

    // Sets the current keyboard instrument based on the user's choice.
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

    // Plays a short keyboard melody to test the selected sound.
    void playKeyboardTest(GlobalState& state)
    {
        const int melodyNotes[] = {60, 64, 67, 72};

        for (int note : melodyNotes)
        {
            pressKeyboardNote(state, note, 100, 400, 100);
        }

        std::cout << "Keyboard test finished!" << std::endl;
    }

    // Sets the current theremin waveform based on the user's choice.
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

    // Configures the system for drum mode and loads the drum sound.
    void setupDrums(GlobalState& state, HeadlessAudioEngine& audio)
    {
        state.currentInstrument.store(ActiveInstrument::Drums);
        audio.loadDrumSound("Instruments/SMDrums_Sforzando_1.2/Programs/SM_Drums_kit.sfz");
        std::cout << "\n>>> DRUMS ACTIVE <<<\n"
                  << "Move either hand into the on-screen drum zones to trigger hits.\n"
                  << std::endl;

        playDrumTest(state);
    }

    // Configures the system for keyboard mode and loads the selected keyboard sound.
    void setupKeyboard(GlobalState& state, HeadlessAudioEngine& audio)
    {
        state.currentInstrument.store(ActiveInstrument::Keyboard);

        const char keyboardSoundChoice = askKeyboardSoundChoice();
        const int keyboardSoundID = configureKeyboardSound(state, keyboardSoundChoice);

        audio.loadKeyboardSound(keyboardSoundID);
        playKeyboardTest(state);
    }

    // Configures the system for theremin mode and sets the selected waveform.
    void setupTheremin(GlobalState& state)
    {
        state.currentInstrument.store(ActiveInstrument::Theremin);
        std::cout << "\n>>> THEREMIN ACTIVE <<<\n" << std::endl;

        const char waveChoice = askThereminWaveChoice();
        configureThereminWaveform(state, waveChoice);
    }

    // Launches the external hand detector script if the required environment variable is set.
    void launchHandDetectorIfRequested(const GlobalState& state)
    {
        if (std::getenv("ULTEVIS_LAUNCH_HAND_DETECTOR") == nullptr)
            return;

        const char* detectorScript = std::getenv("ULTEVIS_HAND_DETECTOR_SCRIPT");
        const std::string scriptPath = detectorScript != nullptr
            ? detectorScript
            : "src\\mediapipe\\hand_detector.py";
        const auto cameraMode = state.currentInstrument.load() == ActiveInstrument::Drums
            ? std::string("drums")
            : std::string("theremin");
#ifdef _WIN32
        const std::string command =
            "set \"ULTEVIS_CAMERA_MODE=" + cameraMode +
            "\" && start \"Ultevis Hand Detector\" python \"" + scriptPath + "\"";
#else
        const std::string command =
            "ULTEVIS_CAMERA_MODE=" + cameraMode + " python3 \"" + scriptPath + "\" &";
#endif
        std::system(command.c_str());
    }

    // Initializes the basic audio test state from user input.
    void initializeAudioTestState(GlobalState& state)
    {
        state.masterVolume.store(askMasterVolume());
        state.routeToMidiOut.store(askEnableMidi());
    }
}

// Runs the interactive audio test menu and launches the selected instrument mode.
int main()
{
    GlobalState state;

    initializeAudioTestState(state);

    HeadlessAudioEngine audio(&state);

    std::cout << "\nBooted!\n" << std::endl;

    while (true)
    {
        const char instChoice = askInstrumentChoice();

        if (instChoice == 'q' || instChoice == 'Q')
        {
            std::cout << "\nExiting.\n" << std::endl;
            break;
        }

        if (instChoice == '1') // Drums
        {
            setupDrums(state, audio);

            std::cout << "\nWaiting For Python Script To Start Camera\n" << std::endl;
            launchHandDetectorIfRequested(state);
            startCameraFeed(&state);

            return 0; // does not return anyway, but explicit
        }
        else if (instChoice == '2') // Keyboard
        {
            setupKeyboard(state, audio);
            std::cout << "\nKeyboard test complete. Returning to menu.\n" << std::endl;
        }
        else // Theremin
        {
            setupTheremin(state);

            std::cout << "\nWaiting For Python Script To Start Camera\n" << std::endl;
            launchHandDetectorIfRequested(state);
            startCameraFeed(&state);

            return 0;
        }
    }

    return 0;
}