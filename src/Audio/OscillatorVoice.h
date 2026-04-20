#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Core/GlobalState.h"

struct SineWaveSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SineWaveVoice : public juce::SynthesiserVoice
{
public:
    SineWaveVoice();

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void updateThereminMath(double targetFrequencyHz, float targetVolume);
    void setWaveform(Waveform newWaveform);
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    double currentAngle = 0.0;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
    juce::SmoothedValue<double> smoothedFrequency;
    juce::SmoothedValue<float> smoothedVolume;
    Waveform currentWaveform = Waveform::Sine;
};