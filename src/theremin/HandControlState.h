#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>

namespace ii1305
{
struct HandControlSnapshot
{
    float x = 0.5f;
    float y = 0.5f;
    bool active = false;
};

class HandControlState
{
public:
    void setFromNormalized(float newX, float newY, bool isActive) noexcept
    {
        x.store(sanitizeNormalized(newX), std::memory_order_relaxed);
        y.store(sanitizeNormalized(newY), std::memory_order_relaxed);
        active.store(isActive, std::memory_order_relaxed);
    }

    HandControlSnapshot snapshot() const noexcept
    {
        return {
            x.load(std::memory_order_relaxed),
            y.load(std::memory_order_relaxed),
            active.load(std::memory_order_relaxed)
        };
    }

    static float sanitizeNormalized(float value) noexcept
    {
        if (!std::isfinite(value))
            return 0.0f;

        return std::clamp(value, 0.0f, 1.0f);
    }

private:
    std::atomic<float> x { 0.5f };
    std::atomic<float> y { 0.5f };
    std::atomic<bool> active { false };
};
}
