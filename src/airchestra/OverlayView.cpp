#include "OverlayView.h"
#include "UiActions.h"

namespace airchestra
{
using namespace theme;

void OverlayView::render(ViewState& state, EventLogger& logger)
{
    const auto displaySize = ImGui::GetIO().DisplaySize;
    constexpr float overlayWidth = 380.0f;
    constexpr float overlayHeight = 480.0f;
    const auto overlayX = juce::jmax(24.0f, displaySize.x - overlayWidth - 24.0f);

    ImGui::SetNextWindowPos(ImVec2(overlayX, 92.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(overlayWidth, overlayHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.92f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 14.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, kOverlayBg);
    ImGui::PushStyleColor(ImGuiCol_Border, kBorder);

    auto overlayOpen = state.debugOverlayVisible;

    constexpr auto overlayFlags = ImGuiWindowFlags_NoCollapse
                                | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Airchestra Debug Overlay", &overlayOpen, overlayFlags))
    {
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
        if (!overlayOpen)
            ui_actions::setOverlayVisible(state, logger, false, "overlay_close");
        return;
    }

    // Header
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.15f);
    ImGui::PushStyleColor(ImGuiCol_Text, kAccent);
    ImGui::TextUnformatted("Debug Dashboard");
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::Spacing();

    // Quick status bar
    ImGui::PushStyleColor(ImGuiCol_Text, kTextSecondary);
    ImGui::Text("Screen: %s", toDisplayName(state.currentScreen));
    ImGui::SameLine(200.0f);

    // Session dot + text
    {
        auto* dl = ImGui::GetWindowDrawList();
        auto pos = ImGui::GetCursorScreenPos();
        pos.y += 5.0f;
        dl->AddCircleFilled(pos, 4.0f,
            ImGui::ColorConvertFloat4ToU32(state.sessionRunning ? kGreen : kRed));
        ImGui::Dummy(ImVec2(12.0f, 0.0f));
        ImGui::SameLine();
        ImGui::Text("Session: %s", state.sessionRunning ? "Running" : "Stopped");
    }
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // Controls row
    if (ImGui::Button("Hide", ImVec2(80.0f, 26.0f)))
        ui_actions::setOverlayVisible(state, logger, false, "overlay_button");

    ImGui::SameLine();
    auto compactOverlay = state.compactOverlay;
    if (ImGui::Checkbox("Compact", &compactOverlay))
    {
        state.compactOverlay = compactOverlay;
        ui_actions::recordSettingChanged(state, logger, "compact_overlay", compactOverlay ? "true" : "false");
    }

    ImGui::Separator();

    // Status sections with colored dots
    renderStatusSection(state, logger, DetailPanel::Camera, "Camera: Not connected", "Tracking: No data");
    renderStatusSection(state, logger, DetailPanel::Audio, "Audio: Not initialized", "MIDI: Not initialized");
    renderStatusSection(state, logger, DetailPanel::UI, "Navigation: Active", "Controls: Start / Settings / About");
    renderStatusSection(state, logger, DetailPanel::SystemState, "Runtime: Dev skeleton", "Deps: JUCE + Dear ImGui");

    if (!state.compactOverlay)
        renderStatusSection(state, logger, DetailPanel::InteractionLog, "Event log: Enabled", "Format: JSONL");

    ImGui::Separator();
    ImGui::Spacing();

    // Interaction log footer
    ImGui::PushStyleColor(ImGuiCol_Text, kTextDim);
    ImGui::Text("Events: %d", state.interactionCount);
    ImGui::SameLine(200.0f);
    ImGui::Text("Last: %s", state.lastInteraction.toRawUTF8());
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, kTextSecondary);
    ImGui::Text("%s", logger.getStatusText().toRawUTF8());
    ImGui::PopStyleColor();

    auto showLogPath = state.showLogPathInOverlay;
    if (ImGui::Checkbox("Show log path", &showLogPath))
    {
        state.showLogPathInOverlay = showLogPath;
        ui_actions::recordSettingChanged(state, logger, "show_log_path_in_overlay", showLogPath ? "true" : "false");
    }

    if (state.showLogPathInOverlay)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, kTextDim);
        ImGui::TextWrapped("%s", logger.getLogFile().getFullPathName().toRawUTF8());
        ImGui::PopStyleColor();
    }

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    if (!overlayOpen)
        ui_actions::setOverlayVisible(state, logger, false, "overlay_close");
}

void OverlayView::renderStatusSection(ViewState& state,
                                      EventLogger& logger,
                                      DetailPanel panel,
                                      const char* line1,
                                      const char* line2)
{
    ImGui::Spacing();

    // Colored dot
    auto* dl = ImGui::GetWindowDrawList();
    auto pos = ImGui::GetCursorScreenPos();
    pos.y += 5.0f;
    dl->AddCircleFilled(pos, 4.0f, panelDotColor(panel));
    ImGui::Dummy(ImVec2(12.0f, 0.0f));
    ImGui::SameLine();

    if (ImGui::Selectable(toDisplayName(panel), state.selectedPanel == panel))
        ui_actions::selectPanel(state, logger, panel, "debug_overlay");

    ImGui::Indent(20.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, kTextDim);
    ImGui::BulletText("%s", line1);
    ImGui::BulletText("%s", line2);
    ImGui::PopStyleColor();
    ImGui::Unindent(20.0f);
}
}
