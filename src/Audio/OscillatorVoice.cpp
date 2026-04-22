/*
==============================================================================
OSCILLATOR VOICE IMPLEMENTATION

Implements the synthesis voice used by the theremin. This file contains the
audio-generation logic for a single voice, including ADSR handling, waveform
generation, smoothing of pitch/volume changes, and block rendering.
==============================================================================
*/

#include "OscillatorVoice.h"
#include <cmath>

// =============================================================================
// OSCILLATOR VOICE IMPLEMENTATION
// =============================================================================
SineWaveVoice::SineWaveVoice()
{
    adsrParams.attack  = 0.005f;  // 5 ms — near-instant onset
    adsrParams.decay   = 0.05f;
    adsrParams.sustain = 1.0f;
    adsrParams.release = 0.15f;   // 150 ms — quick but smooth cutoff
}

bool SineWaveVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<SineWaveSound*>(sound) != nullptr;
}

void SineWaveVoice::startNote(int /*midiNoteNumber*/, float /*velocity*/, juce::SynthesiserSound*, int)
{
    smoothedFrequency.reset(getSampleRate(), 0.02);
    smoothedVolume.reset(getSampleRate(), 0.02);

    adsr.setParameters(adsrParams);
    adsr.noteOn();
}

void SineWaveVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff) adsr.noteOff();
    else clearCurrentNote();
}

void SineWaveVoice::updateThereminMath(double targetFreq, float targetVol)
{
    smoothedFrequency.setTargetValue(targetFreq);
    smoothedVolume.setTargetValue(targetVol);
}

void SineWaveVoice::setWaveform(Waveform newWaveform)
{
    currentWaveform = newWaveform;
}

void SineWaveVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    for (int i = 0; i < numSamples; ++i) {
        double currentFreq = smoothedFrequency.getNextValue();
        float currentVol = smoothedVolume.getNextValue();
        float currentAdsr = adsr.getNextSample();

        auto cyclesPerSample = currentFreq / getSampleRate();
        auto angleDelta = cyclesPerSample * juce::MathConstants<double>::twoPi;

        currentAngle += angleDelta;
        if (currentAngle >= juce::MathConstants<double>::twoPi)
            currentAngle -= juce::MathConstants<double>::twoPi;

        float waveSample = 0.0f;
        double phase = currentAngle / juce::MathConstants<double>::twoPi;

        switch (currentWaveform) {
            case Waveform::Sine:
                waveSample = (float)std::sin(currentAngle);
                break;
            case Waveform::Square:
                waveSample = (std::sin(currentAngle) >= 0.0) ? 1.0f : -1.0f;
                break;
            case Waveform::Saw:
                waveSample = (float)(2.0 * phase - 1.0);
                break;
            case Waveform::Triangle:
                waveSample = (float)(2.0 * std::abs(2.0 * phase - 1.0) - 1.0);
                break;
        }

        float currentSample = waveSample * currentAdsr * currentVol * 0.2f;

        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) {
            outputBuffer.addSample(channel, startSample + i, currentSample);
        }
    }

    if (!adsr.isActive()) clearCurrentNote();
}

// =============================================================================
// KEYBOARD VOICE
// =============================================================================
KeyboardVoice::KeyboardVoice() { adsr.setParameters(adsrParams); }

bool KeyboardVoice::canPlaySound(juce::SynthesiserSound* s)
{
    return dynamic_cast<KeySynthSound*>(s) != nullptr;
}

void KeyboardVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    currentAngle = 0.0;
    double freq  = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    angleDelta   = freq / getSampleRate() * juce::MathConstants<double>::twoPi;
    gain         = velocity;
    adsr.setParameters(adsrParams);
    adsr.noteOn();
}

void KeyboardVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff) adsr.noteOff();
    else              clearCurrentNote();
}

void KeyboardVoice::renderNextBlock(juce::AudioBuffer<float>& buf, int start, int num)
{
    for (int i = 0; i < num; ++i) {
        float sample = (float)(std::sin(currentAngle) * adsr.getNextSample() * gain * 0.25f);
        currentAngle += angleDelta;
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
            buf.addSample(ch, start + i, sample);
    }
    if (!adsr.isActive()) clearCurrentNote();
}

// =============================================================================
// DRUM VOICE
// =============================================================================
DrumVoice::DrumVoice() { adsr.setParameters(adsrParams); }

bool DrumVoice::canPlaySound(juce::SynthesiserSound* s)
{
    return dynamic_cast<DrumSynthSound*>(s) != nullptr;
}

void DrumVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    currentAngle = 0.0;
    double freq  = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    angleDelta   = freq / getSampleRate() * juce::MathConstants<double>::twoPi;
    gain         = velocity;
    adsr.setParameters(adsrParams);
    adsr.noteOn();
}

void DrumVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff) adsr.noteOff();
    else              clearCurrentNote();
}

void DrumVoice::renderNextBlock(juce::AudioBuffer<float>& buf, int start, int num)
{
    for (int i = 0; i < num; ++i) {
        float sample = (float)(std::sin(currentAngle) * adsr.getNextSample() * gain * 0.35f);
        currentAngle += angleDelta;
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
            buf.addSample(ch, start + i, sample);
    }
    if (!adsr.isActive()) clearCurrentNote();
}