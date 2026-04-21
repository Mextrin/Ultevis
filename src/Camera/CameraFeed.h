#pragma once
#include <atomic>
#include "../Core/GlobalState.h"

// Blocks until stopFlag becomes true or an unrecoverable socket error occurs.
// Uses a 100 ms receive timeout so the stop flag is checked regularly.
void startCameraFeed(GlobalState* state, std::atomic<bool>* stopFlag = nullptr);
