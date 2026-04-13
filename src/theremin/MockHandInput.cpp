#include "MockHandInput.h"

#include <juce_core/juce_core.h>

#include <algorithm>
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

void MockHandInput::setEnabled(bool shouldBeEnabled) noexcept
{
    enabled.store(shouldBeEnabled, std::memory_order_relaxed);

    if (!shouldBeEnabled)
    {
        const auto snapshot = state.snapshot();
        state.setFromNormalized(snapshot.x, snapshot.y, false);
    }
}

bool MockHandInput::isEnabled() const noexcept
{
    return enabled.load(std::memory_order_relaxed);
}

void MockHandInput::setMotionScale(float normalizedScale) noexcept
{
    motionScale.store(std::clamp(normalizedScale, 0.1f, 1.0f), std::memory_order_relaxed);
}

void MockHandInput::timerCallback()
{
    const auto nowSeconds = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const auto t = nowSeconds - startTimeSeconds;
    const auto amplitude = 0.45f * motionScale.load(std::memory_order_relaxed);

    const auto x = 0.5f + amplitude * static_cast<float>(std::sin(t * 0.9));
    const auto y = 0.5f + amplitude * static_cast<float>(std::sin(t * 1.3 + 1.1));

    state.setFromNormalized(x, y, enabled.load(std::memory_order_relaxed));
}
}
