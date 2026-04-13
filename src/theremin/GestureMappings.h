#pragma once

#include "HandControlState.h"

#include <cmath>

namespace ii1305::GestureMappings
{
inline constexpr float minFrequencyHz = 220.0f;
inline constexpr float maxFrequencyHz = 1760.0f;
inline constexpr int midiChannel = 1;
inline constexpr int expressionController = 11;

inline float normalizedXToFrequencyHz(float normalizedX) noexcept
{
    const auto x = HandControlState::sanitizeNormalized(normalizedX);
    return minFrequencyHz * std::pow(maxFrequencyHz / minFrequencyHz, x);
}

inline int normalizedXToPitchBend(float normalizedX) noexcept
{
    const auto x = HandControlState::sanitizeNormalized(normalizedX);
    return static_cast<int>(std::lround(x * 16383.0f));
}

inline int normalizedYToExpression(float normalizedY) noexcept
{
    const auto y = HandControlState::sanitizeNormalized(normalizedY);
    return static_cast<int>(std::lround(y * 127.0f));
}
}
