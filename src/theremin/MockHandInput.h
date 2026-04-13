#pragma once

#include "HandControlState.h"

#include <juce_events/juce_events.h>

#include <atomic>

namespace ii1305
{
class MockHandInput final : private juce::Timer
{
public:
    explicit MockHandInput(HandControlState& stateToUpdate);
    ~MockHandInput() override;

    void setEnabled(bool shouldBeEnabled) noexcept;
    bool isEnabled() const noexcept;
    void setMotionScale(float normalizedScale) noexcept;

private:
    void timerCallback() override;

    HandControlState& state;
    double startTimeSeconds = 0.0;
    std::atomic<bool> enabled { true };
    std::atomic<float> motionScale { 1.0f };
};
}
