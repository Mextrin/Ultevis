// Theremin synthesis voice implementation.
// Generates waveform audio with ADSR control,
// smoothing, and block-based rendering.

#include "OscillatorVoice.h"
#include <cmath>

SineWaveVoice::SineWaveVoice()
{
    adsrParams.attack  = 0.1f;
    adsrParams.decay   = 0.1f;
    adsrParams.sustain = 1.0f;
    adsrParams.release = 0.5f;
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