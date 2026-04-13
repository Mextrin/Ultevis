#include "ThereminAudioComponent.h"

#include "GestureMappings.h"

#include <juce_core/juce_core.h>

#include <cmath>

namespace ii1305
{
ThereminAudioComponent::ThereminAudioComponent(HandControlState& controlState)
    : state(controlState)
{
    setAudioChannels(0, 2);
}

ThereminAudioComponent::~ThereminAudioComponent()
{
    shutdownAudio();
}

void ThereminAudioComponent::prepareToPlay(int samplesPerBlockExpected, double newSampleRate)
{
    juce::ignoreUnused(samplesPerBlockExpected);

    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    phase = 0.0f;
    smoothedFrequencyHz = GestureMappings::normalizedXToFrequencyHz(state.snapshot().x);
    smoothedGain = 0.0f;
}

void ThereminAudioComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (bufferToFill.buffer == nullptr)
        return;

    const auto snapshot = state.snapshot();
    const auto targetFrequencyHz = GestureMappings::normalizedXToFrequencyHz(snapshot.x);
    const auto targetGain = snapshot.active ? 0.15f : 0.0f;
    const auto phaseWrap = juce::MathConstants<float>::twoPi;
    const auto frequencySmoothing = 0.0025f;
    const auto gainSmoothing = 0.0015f;

    auto* buffer = bufferToFill.buffer;
    const auto numChannels = buffer->getNumChannels();

    for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
    {
        smoothedFrequencyHz += frequencySmoothing * (targetFrequencyHz - smoothedFrequencyHz);
        smoothedGain += gainSmoothing * (targetGain - smoothedGain);

        const auto value = std::sin(phase) * smoothedGain;
        phase += phaseWrap * smoothedFrequencyHz / static_cast<float>(sampleRate);

        if (phase >= phaseWrap)
            phase -= phaseWrap;

        const auto sampleIndex = bufferToFill.startSample + sample;

        for (int channel = 0; channel < numChannels; ++channel)
            buffer->setSample(channel, sampleIndex, value);
    }
}

void ThereminAudioComponent::releaseResources()
{
}
}
