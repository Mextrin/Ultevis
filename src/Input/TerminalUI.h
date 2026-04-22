#pragma once

class GlobalState;

namespace TerminalUI
{
    float askMasterVolume();
    bool askEnableMidi();
    char askInstrumentChoice();
    char askThereminWaveChoice();
    char askKeyboardSoundChoice();

    void configureThereminWaveform(GlobalState& state, char waveChoice);
    int configureKeyboardSound(GlobalState& state, char choice);

    void playDrumTest(GlobalState& state);
    void playKeyboardTest(GlobalState& state);
}