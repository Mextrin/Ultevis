#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Core/GlobalState.h"
#include "OscillatorVoice.h"
#include <sfizz.hpp>
#include <string>
#include <vector>
#include <utility>

class HeadlessAudioEngine : public juce::AudioIODeviceCallback
{
public:
    explicit HeadlessAudioEngine(GlobalState* statePtr);
    ~HeadlessAudioEngine() override;

    // Returns { identifier, displayName } pairs for all available MIDI outputs.
    std::vector<std::pair<std::string, std::string>> getAvailableMidiDevices() const;

    // Open a specific MIDI output by device identifier. Pass empty string to disable.
    void openMidiDevice(const std::string& identifier);

    void loadDrumSound(const juce::String& sfzPath);
    void loadKeyboardSound(int keyboardInstrumentID);

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(
        const float* const* inputChannelData, int numInputChannels,
        float* const* outputChannelData, int numOutputChannels,
        int numSamples, const juce::AudioIODeviceCallbackContext& context) override;

private:
    void processTheremin(juce::AudioBuffer<float>& buffer, int numSamples);
    void processDrums(juce::AudioBuffer<float>& buffer, int numSamples);
    void processKeyboard(juce::AudioBuffer<float>& buffer, int numSamples);

    juce::AudioDeviceManager deviceManager;
    GlobalState* globalState;
    juce::Synthesiser synth;
    sfz::Sfizz drumSynth;
    sfz::Sfizz keyboardSynth;
    std::unique_ptr<juce::MidiOutput> midiOut;

    bool wasRightVisible      = false;
    bool wasKeyPressed        = false;
    int  lastPlayedKey        = -1;
    bool wasSustainPedalPressed = false;
};
