#pragma once

class GlobalState;
class HeadlessAudioEngine;

namespace AppController
{
    void initializeAudioTestState(GlobalState& state);
    void run(GlobalState& state, HeadlessAudioEngine& audio);

    void setupDrums(GlobalState& state, HeadlessAudioEngine& audio);
    void setupKeyboard(GlobalState& state, HeadlessAudioEngine& audio);
    void setupTheremin(GlobalState& state);

    void launchHandDetectorIfRequested(const GlobalState& state);
}