// Application control layer implementation.
// Orchestrates program flow, user interaction via TerminalUI,
// instrument setup, and camera/hand-tracking startup.

#include "AppController.h"

#include "Core/GlobalState.h"
#include "Audio/AudioEngine.h"
#include "Input/TerminalUI.h"

#include <cstdlib>
#include <iostream>
#include <string>

extern void startCameraFeed(GlobalState* state);

namespace AppController
{
    // Runs the current terminal-based application flow.
    void run(GlobalState& state, HeadlessAudioEngine& audio)
    {
        std::cout << "\nBooted!\n" << std::endl;

        while (true)
        {
            const char instChoice = TerminalUI::askInstrumentChoice();

            if (instChoice == 'q' || instChoice == 'Q')
            {
                std::cout << "\nExiting.\n" << std::endl;
                break;
            }

            if (instChoice == '1')
            {
                setupDrums(state, audio);

                std::cout << "\nWaiting For Python Script To Start Camera\n" << std::endl;
                launchHandDetectorIfRequested(state);
                startCameraFeed(&state);

                return;
            }
            else if (instChoice == '2')
            {
                setupKeyboard(state, audio);
                std::cout << "\nKeyboard test complete. Returning to menu.\n" << std::endl;
            }
            else
            {
                setupTheremin(state);

                std::cout << "\nWaiting For Python Script To Start Camera\n" << std::endl;
                launchHandDetectorIfRequested(state);
                startCameraFeed(&state);

                return;
            }
        }
    }

    // Initializes the basic audio test state from terminal input.
    void initializeAudioTestState(GlobalState& state)
    {
        state.masterVolume.store(TerminalUI::askMasterVolume());
        state.routeToMidiOut.store(TerminalUI::askEnableMidi());
    }

    // Configures the system for drum mode and loads the drum sound.
    void setupDrums(GlobalState& state, HeadlessAudioEngine& audio)
    {
        state.currentInstrument.store(ActiveInstrument::Drums);
        audio.loadDrumSound("Instruments/SMDrums_Sforzando_1.2/Programs/SM_Drums_kit.sfz");

        std::cout << "\n>>> DRUMS ACTIVE <<<\n"
                  << "Move either hand into the on-screen drum zones to trigger hits.\n"
                  << std::endl;

        TerminalUI::playDrumTest(state);
    }

    // Configures the system for keyboard mode and loads the selected keyboard sound.
    void setupKeyboard(GlobalState& state, HeadlessAudioEngine& audio)
    {
        state.currentInstrument.store(ActiveInstrument::Keyboard);

        const char keyboardSoundChoice = TerminalUI::askKeyboardSoundChoice();
        const int keyboardSoundID = TerminalUI::configureKeyboardSound(state, keyboardSoundChoice);

        audio.loadKeyboardSound(keyboardSoundID);
        TerminalUI::playKeyboardTest(state);
    }

    // Configures the system for theremin mode and sets the selected waveform.
    void setupTheremin(GlobalState& state)
    {
        state.currentInstrument.store(ActiveInstrument::Theremin);
        std::cout << "\n>>> THEREMIN ACTIVE <<<\n" << std::endl;

        const char waveChoice = TerminalUI::askThereminWaveChoice();
        TerminalUI::configureThereminWaveform(state, waveChoice);
    }

    // Launches the external hand detector script if the environment variable is set.
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
            //dev null sends mediapipe garbage output to void 
            "ULTEVIS_CAMERA_MODE=" + cameraMode + " python3 \"" + scriptPath + "\" > /dev/null 2>&1 &";
    #endif

        std::system(command.c_str());
    }
}