#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Core/GlobalState.h"

// ==============================================================================
// 1. THE SOUND (The Rulebook)
// ==============================================================================
struct SineWaveSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

// ==============================================================================
// 2. THE VOICE (The Worker)
// ==============================================================================
class SineWaveVoice : public juce::SynthesiserVoice
{
public:
    SineWaveVoice();

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int /*currentPitchWheelPosition*/) override;
    void stopNote (float /*velocity*/, bool allowTailOff) override;
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    // Custom Theremin Function: Called by the Audio Thread to update math instantly
    void updateThereminMath(double targetFrequencyHz, float targetVolume);

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    double currentAngle = 0.0;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;

    // Smoothed values prevent audio "glitches" between 30FPS camera frames
    juce::SmoothedValue<double> smoothedFrequency;
    juce::SmoothedValue<float> smoothedVolume;
};

// ==============================================================================
// 3. THE AUDIO ENGINE (The Manager)
// ==============================================================================
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

private:
    juce::AudioDeviceManager deviceManager;
    GlobalState* globalState; 
    juce::Synthesiser synth;
    std::unique_ptr<juce::MidiOutput> midiOut;

    bool wasRightVisible = false; // Prevents triggering the note 340 times a second
};