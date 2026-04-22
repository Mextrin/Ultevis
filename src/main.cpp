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

    // Build startup settings for the audio engine.
    AudioEngineConfig config = AppController::buildAudioEngineConfig(state);

    // Create engine from the chosen settings.
    HeadlessAudioEngine audio(&state, config);

    // Run app loop.
    AppController::run(state, audio);

    return 0;
}