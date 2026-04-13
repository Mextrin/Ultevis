#pragma once

#include "HandControlState.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_events/juce_events.h>

#include <memory>

namespace ii1305
{
class MidiGestureOutput final : private juce::Timer
{
public:
    explicit MidiGestureOutput(HandControlState& controlState);
    ~MidiGestureOutput() override;

    juce::String getStatusText() const;
    juce::String getOpenedDeviceName() const;
    bool isOpen() const noexcept;

private:
    void timerCallback() override;
    void openPreferredDevice();
    static juce::MidiDeviceInfo choosePreferredDevice(const juce::Array<juce::MidiDeviceInfo>& devices);

    HandControlState& state;
    std::unique_ptr<juce::MidiOutput> midiOutput;
    juce::String openedDeviceName;
    juce::String statusText;
    int lastPitchBend = -1;
    int lastExpression = -1;
    int retryTicks = 0;
};
}
