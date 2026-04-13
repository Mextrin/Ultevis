#pragma once

#include "HandControlState.h"

#include <juce_audio_utils/juce_audio_utils.h>

namespace ii1305
{
class ThereminAudioComponent final : public juce::AudioAppComponent
{
public:
    explicit ThereminAudioComponent(HandControlState& controlState);
    ~ThereminAudioComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double newSampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

private:
    HandControlState& state;
    double sampleRate = 44100.0;
    float phase = 0.0f;
    float smoothedFrequencyHz = 440.0f;
    float smoothedGain = 0.0f;
};
}
