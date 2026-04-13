#pragma once

#include <imgui.h>
#include <juce_core/juce_core.h>

namespace airchestra
{

enum class AppScreen
{
    Landing,
    Session,
    Settings,
    About
};

enum class DetailPanel
{
    Camera,
    Audio,
    UI,
    SystemState,
    InteractionLog
};

namespace theme
{
    inline constexpr ImVec4 kBgDark        {0.063f, 0.071f, 0.086f, 1.0f};
    inline constexpr ImVec4 kBgPanel       {0.098f, 0.106f, 0.130f, 1.0f};
    inline constexpr ImVec4 kBgChild       {0.114f, 0.122f, 0.149f, 1.0f};
    inline constexpr ImVec4 kBgFrame       {0.137f, 0.149f, 0.180f, 1.0f};
    inline constexpr ImVec4 kAccent        {0.204f, 0.596f, 0.859f, 1.0f};
    inline constexpr ImVec4 kAccentHover   {0.259f, 0.647f, 0.890f, 1.0f};
    inline constexpr ImVec4 kAccentActive  {0.161f, 0.502f, 0.725f, 1.0f};
    inline constexpr ImVec4 kAccentDim     {0.204f, 0.596f, 0.859f, 0.30f};
    inline constexpr ImVec4 kTextPrimary   {0.922f, 0.929f, 0.941f, 1.0f};
    inline constexpr ImVec4 kTextSecondary {0.580f, 0.600f, 0.647f, 1.0f};
    inline constexpr ImVec4 kTextDim       {0.420f, 0.440f, 0.490f, 1.0f};
    inline constexpr ImVec4 kBorder        {0.200f, 0.216f, 0.255f, 0.50f};
    inline constexpr ImVec4 kBorderAccent  {0.204f, 0.596f, 0.859f, 0.60f};
    inline constexpr ImVec4 kGreen         {0.180f, 0.745f, 0.486f, 1.0f};
    inline constexpr ImVec4 kOrange        {0.902f, 0.584f, 0.208f, 1.0f};
    inline constexpr ImVec4 kRed           {0.878f, 0.278f, 0.278f, 1.0f};
    inline constexpr ImVec4 kOverlayBg     {0.075f, 0.082f, 0.102f, 0.92f};

    inline ImU32 panelDotColor(DetailPanel panel)
    {
        switch (panel)
        {
            case DetailPanel::Camera:         return IM_COL32(52, 152, 219, 255);
            case DetailPanel::Audio:          return IM_COL32(46, 204, 113, 255);
            case DetailPanel::UI:             return IM_COL32(155, 89, 182, 255);
            case DetailPanel::SystemState:    return IM_COL32(230, 149, 53, 255);
            case DetailPanel::InteractionLog: return IM_COL32(231, 76, 60, 255);
        }
        return IM_COL32(128, 128, 128, 255);
    }
}

inline const char* toDisplayName(AppScreen screen) noexcept
{
    switch (screen)
    {
        case AppScreen::Landing: return "Landing";
        case AppScreen::Session: return "Control Room";
        case AppScreen::Settings: return "Settings";
        case AppScreen::About: return "About";
    }

    return "Unknown";
}

inline const char* toDisplayName(DetailPanel panel) noexcept
{
    switch (panel)
    {
        case DetailPanel::Camera: return "Camera";
        case DetailPanel::Audio: return "Audio";
        case DetailPanel::UI: return "UI";
        case DetailPanel::SystemState: return "System State";
        case DetailPanel::InteractionLog: return "Interaction Log";
    }

    return "Unknown";
}

struct ViewState
{
    bool debugOverlayVisible = false;
    bool landingPageShownLogged = false;
    bool sessionRunning = false;
    bool compactOverlay = false;
    bool showLogPathInOverlay = true;
    bool simulateInputPreview = true;
    bool cameraPreviewEnabled = false;
    bool audioPlaceholderArmed = false;
    bool midiPlaceholderArmed = false;
    bool highContrastUi = false;
    float mockSensitivity = 0.65f;
    AppScreen currentScreen = AppScreen::Landing;
    DetailPanel selectedPanel = DetailPanel::SystemState;
    int interactionCount = 0;
    juce::String appStatus = "Ready";
    juce::String lastInteraction = "App launched";
};
}
