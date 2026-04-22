// Application entry point.
// Initializes JUCE, constructs shared state and audio engine,
// and delegates execution to AppController.

#include "Core/GlobalState.h"
#include "Audio/AudioEngine.h"
#include "App/AppController.h"

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    GlobalState state;

    // Ask user settings (volume + midi)
    AppController::initializeAudioTestState(state);

    // Create engine
    HeadlessAudioEngine audio(&state);

    // Run app loop
    AppController::run(state, audio);

    return 0;
}