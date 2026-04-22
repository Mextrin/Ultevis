#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Core/GlobalState.h"

// ---------------------------------------------------------------------------
// Theremin voice
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Keyboard voice — MIDI-pitch-driven, musical ADSR
// ---------------------------------------------------------------------------
struct KeySynthSound : public juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class KeyboardVoice : public juce::SynthesiserVoice
{
public:
    KeyboardVoice();
    bool canPlaySound(juce::SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override;
    void stopNote(float, bool allowTailOff) override;
    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}
    void renderNextBlock(juce::AudioBuffer<float>&, int startSample, int numSamples) override;

private:
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams { 0.01f, 0.15f, 0.75f, 0.4f };
    double currentAngle = 0.0;
    double angleDelta   = 0.0;
    float  gain         = 0.0f;
};

// ---------------------------------------------------------------------------
// Drum voice — percussive ADSR, MIDI-pitch-driven sine burst
// ---------------------------------------------------------------------------
struct DrumSynthSound : public juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class DrumVoice : public juce::SynthesiserVoice
{
public:
    DrumVoice();
    bool canPlaySound(juce::SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override;
    void stopNote(float, bool allowTailOff) override;
    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}
    void renderNextBlock(juce::AudioBuffer<float>&, int startSample, int numSamples) override;

private:
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams { 0.001f, 0.07f, 0.0f, 0.04f };
    double currentAngle = 0.0;
    double angleDelta   = 0.0;
    float  gain         = 0.0f;
};