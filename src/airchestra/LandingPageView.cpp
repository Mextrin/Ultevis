#include "LandingPageView.h"
#include "UiActions.h"

namespace airchestra
{
using namespace theme;

void LandingPageView::render(ViewState& state, EventLogger& logger)
{
    const auto region = ImGui::GetContentRegionAvail();
    const float centerX = region.x * 0.5f;

    // Push content toward vertical center
    ImGui::Dummy(ImVec2(0.0f, region.y * 0.12f));

    // Large title
    {
        ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 2.5f);
        const char* title = "Airchestra";
        const auto titleSize = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX(centerX - titleSize.x * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, kAccent);
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    // Decorative accent line
    {
        auto* drawList = ImGui::GetWindowDrawList();
        const auto cursorScreen = ImGui::GetCursorScreenPos();
        const auto windowPos = ImGui::GetWindowPos();
        const float lineY = cursorScreen.y + 2.0f;
        const float lineCenterX = windowPos.x + centerX;
        drawList->AddLine(
            ImVec2(lineCenterX - 50.0f, lineY),
            ImVec2(lineCenterX + 50.0f, lineY),
            ImGui::ColorConvertFloat4ToU32(kAccent), 2.0f);
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
    }

    // Subtitle
    {
        ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.2f);
        const char* subtitle = "Gesture-Controlled MIDI Performance";
        const auto subSize = ImGui::CalcTextSize(subtitle);
        ImGui::SetCursorPosX(centerX - subSize.x * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, kTextSecondary);
        ImGui::TextUnformatted(subtitle);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    // Description
    {
        const float wrapWidth = juce::jmin(560.0f, region.x * 0.8f);
        ImGui::SetCursorPosX(centerX - wrapWidth * 0.5f);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrapWidth);
        ImGui::PushStyleColor(ImGuiCol_Text, kTextDim);
        ImGui::TextWrapped("A KTH II1305 desktop prototype for exploring hand gestures as musical control data. Start the mock x/y session to hear a sine theremin tone and send MIDI pitch bend plus CC11 expression.");
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos();
    }

    ImGui::Dummy(ImVec2(0.0f, 28.0f));

    // Primary "Start" button - accent colored, centered
    {
        ImGui::PushStyleColor(ImGuiCol_Button, kAccent);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kAccentHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, kAccentActive);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

        const ImVec2 startSize(200.0f, 48.0f);
        ImGui::SetCursorPosX(centerX - startSize.x * 0.5f);
        ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.3f);
        if (ImGui::Button("Start", startSize))
        {
            ui_actions::recordButtonClick(state, logger, "start", "Starting Airchestra session");
            ui_actions::setSessionRunning(state, logger, true, "landing_start");
            ui_actions::changeScreen(state, logger, AppScreen::Session, "landing_start");
        }
        ImGui::PopFont();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
    }

    ImGui::Dummy(ImVec2(0.0f, 14.0f));

    // Secondary buttons row
    {
        const ImVec2 secSize(130.0f, 36.0f);
        const float rowWidth = secSize.x * 2.0f + 12.0f;
        ImGui::SetCursorPosX(centerX - rowWidth * 0.5f);
        renderNavigationButton("Settings", AppScreen::Settings, state, logger);
        ImGui::SameLine(0.0f, 12.0f);
        renderNavigationButton("About", AppScreen::About, state, logger);
    }

    ImGui::Dummy(ImVec2(0.0f, 16.0f));

    // Ghost overlay toggle
    {
        const char* overlayLabel = state.debugOverlayVisible ? "Hide Debug Overlay" : "Show Debug Overlay";
        const auto labelSize = ImGui::CalcTextSize(overlayLabel);
        ImGui::SetCursorPosX(centerX - (labelSize.x + 28.0f) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.06f));
        ImGui::PushStyleColor(ImGuiCol_Text, kTextSecondary);
        if (ImGui::Button(overlayLabel))
            ui_actions::setOverlayVisible(state, logger, !state.debugOverlayVisible, "landing");
        ImGui::PopStyleColor(3);
    }

    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    // Status text centered
    {
        const auto* statusText = state.appStatus.toRawUTF8();
        char buf[128];
        snprintf(buf, sizeof(buf), "Status: %s", statusText);
        const auto statusSize = ImGui::CalcTextSize(buf);
        ImGui::SetCursorPosX(centerX - statusSize.x * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, kTextDim);
        ImGui::TextUnformatted(buf);
        ImGui::PopStyleColor();
    }
}

void LandingPageView::renderNavigationButton(const char* label,
                                             AppScreen targetScreen,
                                             ViewState& state,
                                             EventLogger& logger)
{
    if (ImGui::Button(label, ImVec2(130.0f, 36.0f)))
    {
        ui_actions::recordButtonClick(state, logger, label, juce::String("Opening ") + toDisplayName(targetScreen));
        ui_actions::changeScreen(state, logger, targetScreen, label);
    }
}
}
