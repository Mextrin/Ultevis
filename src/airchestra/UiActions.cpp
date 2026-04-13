#include "UiActions.h"

namespace airchestra::ui_actions
{
void recordButtonClick(ViewState& state,
                       EventLogger& logger,
                       const char* buttonName,
                       const juce::String& statusText)
{
    state.appStatus = statusText;
    state.lastInteraction = juce::String(buttonName) + " clicked";
    ++state.interactionCount;

    logger.log(AppEventType::ButtonClicked,
               { { "button", buttonName },
                 { "status", state.appStatus } });
}

void changeScreen(ViewState& state,
                  EventLogger& logger,
                  AppScreen nextScreen,
                  const char* source)
{
    if (state.currentScreen == nextScreen)
        return;

    const auto previousScreen = state.currentScreen;
    state.currentScreen = nextScreen;
    state.appStatus = juce::String("Showing ") + toDisplayName(nextScreen);
    state.lastInteraction = juce::String("Screen changed to ") + toDisplayName(nextScreen);

    logger.log(AppEventType::ScreenChanged,
               { { "from", toDisplayName(previousScreen) },
                 { "to", toDisplayName(nextScreen) },
                 { "source", source } });
}

void selectPanel(ViewState& state,
                 EventLogger& logger,
                 DetailPanel panel,
                 const char* source)
{
    if (state.selectedPanel == panel)
        return;

    state.selectedPanel = panel;
    state.appStatus = juce::String("Selected ") + toDisplayName(panel);
    state.lastInteraction = juce::String("Selected panel: ") + toDisplayName(panel);
    ++state.interactionCount;

    logger.log(AppEventType::PanelSelected,
               { { "panel", toDisplayName(panel) },
                 { "source", source } });
}

void setOverlayVisible(ViewState& state,
                       EventLogger& logger,
                       bool visible,
                       const char* source)
{
    if (state.debugOverlayVisible == visible)
        return;

    state.debugOverlayVisible = visible;
    state.appStatus = visible ? "Debug overlay shown" : "Debug overlay hidden";
    state.lastInteraction = visible ? "Debug overlay shown" : "Debug overlay hidden";
    ++state.interactionCount;

    logger.log(AppEventType::OverlayToggled,
               { { "visible", visible ? "true" : "false" },
                 { "source", source } });
}

void setSessionRunning(ViewState& state,
                       EventLogger& logger,
                       bool running,
                       const char* source)
{
    if (state.sessionRunning == running)
        return;

    state.sessionRunning = running;
    state.appStatus = running ? "Session running: mock x/y drives audio and MIDI" : "Session stopped: audio muted";
    state.lastInteraction = running ? "Session started" : "Session stopped";
    state.mockInputActive = running && state.simulateInputPreview;
    ++state.interactionCount;

    logger.log(AppEventType::SessionStateChanged,
               { { "running", running ? "true" : "false" },
                 { "source", source } });
}

void recordSettingChanged(ViewState& state,
                          EventLogger& logger,
                          const char* settingName,
                          const juce::String& value)
{
    state.appStatus = juce::String("Updated setting: ") + settingName;
    state.lastInteraction = juce::String(settingName) + " set to " + value;
    ++state.interactionCount;

    logger.log(AppEventType::SettingChanged,
               { { "setting", settingName },
                 { "value", value } });
}
}
