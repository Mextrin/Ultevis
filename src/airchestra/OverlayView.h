#pragma once

#include "EventLogger.h"
#include "ViewState.h"

#include <imgui.h>

namespace airchestra
{
class OverlayView final
{
public:
    void render(ViewState& state, EventLogger& logger);

private:
    static void renderStatusSection(ViewState& state,
                                    EventLogger& logger,
                                    DetailPanel panel,
                                    const char* line1,
                                    const char* line2);
};
}
