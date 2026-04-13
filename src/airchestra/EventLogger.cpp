#include "EventLogger.h"

namespace airchestra
{
EventLogger::EventLogger()
{
    const auto executable = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    const auto logDirectory = executable.getParentDirectory().getChildFile("logs");
    const auto createResult = logDirectory.createDirectory();

    logFile = logDirectory.getChildFile("airchestra-events.jsonl");
    ready = createResult.wasOk();
    statusText = ready ? "Logging to " + logFile.getFullPathName()
                       : "Logging unavailable: " + createResult.getErrorMessage();
}

void EventLogger::log(AppEventType eventType, EventFields fields)
{
    const juce::ScopedLock scopedLock(lock);

    if (!ready)
        return;

    juce::String line;
    line << "{\"timestamp\":\"" << escapeJson(juce::Time::getCurrentTime().toISO8601(true)) << "\"";
    line << ",\"event\":\"" << toEventName(eventType) << "\"";

    for (const auto& field : fields)
        line << ",\"" << escapeJson(field.first) << "\":\"" << escapeJson(field.second) << "\"";

    line << "}";

    if (!logFile.appendText(line + "\n", false, false, "\n"))
    {
        ready = false;
        statusText = "Logging disabled after write failure: " + logFile.getFullPathName();
    }
}

bool EventLogger::isReady() const noexcept
{
    return ready;
}

juce::String EventLogger::getStatusText() const
{
    const juce::ScopedLock scopedLock(lock);
    return statusText;
}

juce::File EventLogger::getLogFile() const
{
    const juce::ScopedLock scopedLock(lock);
    return logFile;
}

const char* EventLogger::toEventName(AppEventType eventType) noexcept
{
    switch (eventType)
    {
        case AppEventType::AppStarted: return "app_started";
        case AppEventType::MainWindowCreated: return "main_window_created";
        case AppEventType::ImGuiInitialized: return "imgui_initialized";
        case AppEventType::LandingPageShown: return "landing_page_shown";
        case AppEventType::ButtonClicked: return "button_clicked";
        case AppEventType::OverlayToggled: return "overlay_toggled";
        case AppEventType::ScreenChanged: return "screen_changed";
        case AppEventType::PanelSelected: return "panel_selected";
        case AppEventType::SettingChanged: return "setting_changed";
        case AppEventType::SessionStateChanged: return "session_state_changed";
        case AppEventType::AppClosing: return "app_closing";
    }

    return "unknown";
}

juce::String EventLogger::escapeJson(const juce::String& text)
{
    juce::String escaped;

    for (int i = 0; i < text.length(); ++i)
    {
        const auto c = text[i];

        if (c == '\\')
            escaped << "\\\\";
        else if (c == '"')
            escaped << "\\\"";
        else if (c == '\n')
            escaped << "\\n";
        else if (c == '\r')
            escaped << "\\r";
        else if (c == '\t')
            escaped << "\\t";
        else
            escaped << juce::String::charToString(c);
    }

    return escaped;
}
}
