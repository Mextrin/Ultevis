#pragma once

#include "EventLogger.h"
#include "ViewState.h"

namespace airchestra::ui_actions
{
void recordButtonClick(ViewState& state,
                       EventLogger& logger,
                       const char* buttonName,
                       const juce::String& statusText);

void changeScreen(ViewState& state,
                  EventLogger& logger,
                  AppScreen nextScreen,
                  const char* source);

void selectPanel(ViewState& state,
                 EventLogger& logger,
                 DetailPanel panel,
                 const char* source);

void setOverlayVisible(ViewState& state,
                       EventLogger& logger,
                       bool visible,
                       const char* source);

void setSessionRunning(ViewState& state,
                       EventLogger& logger,
                       bool running,
                       const char* source);

void recordSettingChanged(ViewState& state,
                          EventLogger& logger,
                          const char* settingName,
                          const juce::String& value);
}
