#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Core/GlobalState.h"
#include "OscillatorVoice.h"
#include <string>
#include <vector>
#include <utility>

class HeadlessAudioEngine : public juce::AudioIODeviceCallback
{
public:
    explicit HeadlessAudioEngine(GlobalState* statePtr);
    ~HeadlessAudioEngine() override;

    std::vector<std::pair<std::string, std::string>> getAvailableMidiDevices() const;
    void openMidiDevice(const std::string& identifier);

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

    juce::Synthesiser thereminSynth;   // 1 voice, continuous freq updates
    juce::Synthesiser keySynth;        // 8 voices, MIDI-pitch-driven
    juce::Synthesiser drumSynth;       // 8 voices, percussive

    std::unique_ptr<juce::MidiOutput> midiOut;

    bool wasRightVisible        = false;
    bool wasKeyPressed          = false;
    int  lastPlayedKey          = -1;
    bool wasSustainPedalPressed = false;

    // How many consecutive audio callbacks the right hand has been absent before
    // we call noteOff.  At ~172 callbacks/s this gives ~47 ms of tolerance,
    // absorbing the occasional missed MediaPipe frame without restarting the note.
    int  thereminInvisibleCount  = 0;
    static constexpr int kInvisibleDebounce = 8;
};
