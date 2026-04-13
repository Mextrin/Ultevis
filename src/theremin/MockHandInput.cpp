#include "MockHandInput.h"

#include <juce_core/juce_core.h>

#include <cmath>

namespace ii1305
{
MockHandInput::MockHandInput(HandControlState& stateToUpdate)
    : state(stateToUpdate),
      startTimeSeconds(juce::Time::getMillisecondCounterHiRes() * 0.001)
{
    startTimerHz(60);
}

MockHandInput::~MockHandInput()
{
    stopTimer();
}

void MockHandInput::timerCallback()
{
    const auto nowSeconds = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const auto t = nowSeconds - startTimeSeconds;

    const auto x = 0.5f + 0.45f * static_cast<float>(std::sin(t * 0.9));
    const auto y = 0.5f + 0.45f * static_cast<float>(std::sin(t * 1.3 + 1.1));

    state.setFromNormalized(x, y, true);
}
}
