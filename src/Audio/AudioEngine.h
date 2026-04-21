#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Core/GlobalState.h"
#include "OscillatorVoice.h"
#include <sfizz.hpp>

class HeadlessAudioEngine : public juce::AudioIODeviceCallback
{
public:
    HeadlessAudioEngine(GlobalState* statePtr);
    ~HeadlessAudioEngine() override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(
        const float* const* inputChannelData, int numInputChannels,
        float* const* outputChannelData, int numOutputChannels,
        int numSamples, const juce::AudioIODeviceCallbackContext& context) override;

    void loadDrumSound(const juce::String& sfzPath);
    void loadKeyboardSound(int keyboardInstrumentID);
    void processTheremin(juce::AudioBuffer<float>& buffer, int numSamples);
    void processDrums(juce::AudioBuffer<float>& buffer, int numSamples);
    void processKeyboard(juce::AudioBuffer<float>& buffer, int numSamples);

private:
    juce::AudioDeviceManager deviceManager;
    GlobalState* globalState;
    juce::Synthesiser synth;
    sfz::Sfizz drumSynth;
    sfz::Sfizz keyboardSynth;
    std::unique_ptr<juce::MidiOutput> midiOut;

    bool wasRightVisible = false;
    bool wasKeyPressed = false;
    int lastPlayedKey = -1; // -1 means no key is playing
};