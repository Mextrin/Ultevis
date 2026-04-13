#pragma once

#include "EventLogger.h"
#include "ViewState.h"

#include <imgui.h>

namespace airchestra
{
class LandingPageView final
{
public:
    void render(ViewState& state, EventLogger& logger);

private:
    static void renderNavigationButton(const char* label,
                                       AppScreen targetScreen,
                                       ViewState& state,
                                       EventLogger& logger);
};
}
