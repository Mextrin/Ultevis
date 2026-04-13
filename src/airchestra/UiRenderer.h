#pragma once

#include "EventLogger.h"
#include "LandingPageView.h"
#include "OverlayView.h"
#include "ViewState.h"

namespace airchestra
{
class UiRenderer final
{
public:
    UiRenderer(ViewState& viewState, EventLogger& eventLogger);

    void render();

private:
    ViewState& state;
    EventLogger& logger;
    LandingPageView landingPage;
    OverlayView overlay;
};
}
