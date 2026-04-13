#pragma once

#include "EventLogger.h"
#include "ImGuiLayer.h"
#include "UiRenderer.h"
#include "ViewState.h"

#include <juce_gui_extra/juce_gui_extra.h>

namespace airchestra
{
class MainComponent final : public juce::Component
{
public:
    explicit MainComponent(EventLogger& eventLogger);

    void resized() override;

private:
    EventLogger& logger;
    ViewState viewState;
    UiRenderer uiRenderer;
    ImGuiLayer imguiLayer;
};
}
