#pragma once

#include "HandControlState.h"
#include "MidiGestureOutput.h"
#include "MockHandInput.h"
#include "ThereminAudioComponent.h"

#include <juce_gui_extra/juce_gui_extra.h>

namespace ii1305
{
class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void configureLabels();
    void updateReadout();

    HandControlState handState;
    ThereminAudioComponent audioEngine;
    MockHandInput mockInput;
    MidiGestureOutput midiOutput;

    juce::Label titleLabel;
    juce::Label readoutLabel;
    juce::Label midiLabel;
    juce::Label instructionLabel;
};
}
