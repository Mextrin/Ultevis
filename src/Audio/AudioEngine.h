/*
==============================================================================
AUDIO ENGINE HEADER (Interface & System Structure)

This file defines the core components of the theremin audio system and their
relationships. It specifies the classes, their roles, and the interfaces they
expose, without describing how their behavior is implemented.

SineWaveSound:
A minimal JUCE SynthesiserSound that acts as a compatibility layer with the
JUCE synthesiser framework. It represents a sound type that can be played by
voices, without imposing restrictions on notes or channels.

SineWaveVoice:
Represents a single synthesis voice responsible for producing audio. It exposes
methods for handling note lifecycle events (start and stop), updating continuous
control parameters (pitch and volume), and rendering audio blocks. It also
defines the internal state required for waveform generation, envelope shaping,
and parameter smoothing.

HeadlessAudioEngine:
Defines the interface for the main audio controller. It connects the JUCE audio
device system with the synthesiser and shared GlobalState. Its role is to manage
audio device lifecycle, coordinate synthesis, and provide a bridge between
external control data and sound generation.

Overall, this file establishes the structure of the audio subsystem, separating
declarations from implementation, which is provided in the corresponding
.cpp file.
==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Core/GlobalState.h"
#include <sfizz.hpp>

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

    void loadDrumSound(const juce::String& sfzPath);

private:
    juce::AudioDeviceManager deviceManager;
    GlobalState* globalState; 
    juce::Synthesiser synth;
    sfz::Sfizz drumSynth;
    std::unique_ptr<juce::MidiOutput> midiOut;

    bool wasRightVisible = false; // Prevents triggering the note 340 times a second
    bool wasDrumHit = false; // Info for drum rolls 
};