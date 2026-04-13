#include "MidiGestureOutput.h"

#include "GestureMappings.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace ii1305
{
MidiGestureOutput::MidiGestureOutput(HandControlState& controlState)
    : state(controlState)
{
    openPreferredDevice();
    startTimerHz(30);
}

MidiGestureOutput::~MidiGestureOutput()
{
    stopTimer();
}

juce::String MidiGestureOutput::getStatusText() const
{
    return statusText;
}

juce::String MidiGestureOutput::getOpenedDeviceName() const
{
    return openedDeviceName;
}

bool MidiGestureOutput::isOpen() const noexcept
{
    return midiOutput != nullptr;
}

void MidiGestureOutput::timerCallback()
{
    if (midiOutput == nullptr)
    {
        if (++retryTicks >= 60)
        {
            retryTicks = 0;
            openPreferredDevice();
        }

        return;
    }

    const auto snapshot = state.snapshot();
    const auto pitchBend = GestureMappings::normalizedXToPitchBend(snapshot.x);
    const auto expression = snapshot.active ? GestureMappings::normalizedYToExpression(snapshot.y) : 0;

    if (pitchBend != lastPitchBend)
    {
        midiOutput->sendMessageNow(juce::MidiMessage::pitchWheel(GestureMappings::midiChannel, pitchBend));
        lastPitchBend = pitchBend;
    }

    if (expression != lastExpression)
    {
        midiOutput->sendMessageNow(juce::MidiMessage::controllerEvent(GestureMappings::midiChannel,
                                                                      GestureMappings::expressionController,
                                                                      expression));
        lastExpression = expression;
    }
}

void MidiGestureOutput::openPreferredDevice()
{
    const auto devices = juce::MidiOutput::getAvailableDevices();

    if (devices.isEmpty())
    {
        midiOutput.reset();
        openedDeviceName = {};
        statusText = "No MIDI outputs available. Start loopMIDI on Windows or enable IAC Driver on macOS, then relaunch or wait for retry.";
        return;
    }

    const auto chosenDevice = choosePreferredDevice(devices);
    midiOutput = juce::MidiOutput::openDevice(chosenDevice.identifier);

    if (midiOutput == nullptr)
    {
        openedDeviceName = {};
        statusText = "Found MIDI outputs, but JUCE could not open the selected device.";
        return;
    }

    openedDeviceName = chosenDevice.name;
    statusText = "MIDI out: " + openedDeviceName;
    lastPitchBend = -1;
    lastExpression = -1;
}

juce::MidiDeviceInfo MidiGestureOutput::choosePreferredDevice(const juce::Array<juce::MidiDeviceInfo>& devices)
{
    for (const auto& device : devices)
    {
        if (device.name.containsIgnoreCase("loopMIDI"))
            return device;
    }

    for (const auto& device : devices)
    {
        if (device.name.containsIgnoreCase("IAC"))
            return device;
    }

    return devices.getFirst();
}
}
