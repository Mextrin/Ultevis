#include "Core/GlobalState.h"
#include "Audio/AudioEngine.h"
#include "Input/TerminalUI.h"
#include "App/AppController.h"

#include <iostream>

extern void startCameraFeed(GlobalState* state);

// Runs the interactive audio test menu and launches the selected instrument mode.
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    GlobalState state;

    AppController::initializeAudioTestState(state);

    HeadlessAudioEngine audio(&state);

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
            AppController::setupDrums(state, audio);

            std::cout << "\nWaiting For Python Script To Start Camera\n" << std::endl;
            AppController::launchHandDetectorIfRequested(state);
            startCameraFeed(&state);

            return 0;
        }
        else if (instChoice == '2')
        {
            AppController::setupKeyboard(state, audio);
            std::cout << "\nKeyboard test complete. Returning to menu.\n" << std::endl;
        }
        else
        {
            AppController::setupTheremin(state);

            std::cout << "\nWaiting For Python Script To Start Camera\n" << std::endl;
            AppController::launchHandDetectorIfRequested(state);
            startCameraFeed(&state);

            return 0;
        }
    }

    return 0;
}