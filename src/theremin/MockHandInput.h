#pragma once

#include "HandControlState.h"

#include <juce_events/juce_events.h>

namespace ii1305
{
class MockHandInput final : private juce::Timer
{
public:
    explicit MockHandInput(HandControlState& stateToUpdate);
    ~MockHandInput() override;

private:
    void timerCallback() override;

    HandControlState& state;
    double startTimeSeconds = 0.0;
};
}
