#pragma once

#include "EventLogger.h"
#include "ImGuiLayer.h"
#include "UiRenderer.h"
#include "ViewState.h"

#include "../theremin/HandControlState.h"
#include "../theremin/MidiGestureOutput.h"
#include "../theremin/MockHandInput.h"
#include "../theremin/ThereminAudioComponent.h"

#include <juce_gui_extra/juce_gui_extra.h>

namespace airchestra
{
class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    explicit MainComponent(EventLogger& eventLogger);
    ~MainComponent() override;

    void resized() override;

private:
    void timerCallback() override;
    void updateThereminReadout();

    EventLogger& logger;
    ii1305::HandControlState handState;
    ii1305::ThereminAudioComponent thereminAudio;
    ii1305::MockHandInput mockInput;
    ii1305::MidiGestureOutput midiOutput;
    ViewState viewState;
    UiRenderer uiRenderer;
    ImGuiLayer imguiLayer;
    juce::String lastMidiStatus;
    bool lastMidiOpen = false;
};
}
