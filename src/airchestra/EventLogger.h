#pragma once

#include <juce_core/juce_core.h>

#include <initializer_list>
#include <utility>

namespace airchestra
{
enum class AppEventType
{
    AppStarted,
    MainWindowCreated,
    ImGuiInitialized,
    LandingPageShown,
    ButtonClicked,
    OverlayToggled,
    ScreenChanged,
    PanelSelected,
    SettingChanged,
    SessionStateChanged,
    MidiStatusChanged,
    AppClosing
};

using EventFields = std::initializer_list<std::pair<juce::String, juce::String>>;

class EventLogger final
{
public:
    EventLogger();

    void log(AppEventType eventType, EventFields fields = {});

    bool isReady() const noexcept;
    juce::String getStatusText() const;
    juce::File getLogFile() const;

private:
    static const char* toEventName(AppEventType eventType) noexcept;
    static juce::String escapeJson(const juce::String& text);

    juce::CriticalSection lock;
    juce::File logFile;
    juce::String statusText;
    bool ready = false;
};
}
