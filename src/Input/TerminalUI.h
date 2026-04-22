#include <string>
#include <vector>
#pragma once

class GlobalState;

namespace TerminalUI
{
    float askMasterVolume();
    bool askEnableMidi();
    char askInstrumentChoice();
    char askThereminWaveChoice();
    char askKeyboardSoundChoice();

    int askMidiDeviceIndex(const std::vector<std::string>& deviceNames);
    void printMidiStatus(bool requested, bool enabled);

    void configureThereminWaveform(GlobalState& state, char waveChoice);
    int configureKeyboardSound(GlobalState& state, char choice);

    void playDrumTest(GlobalState& state);
    void playKeyboardTest(GlobalState& state);
}