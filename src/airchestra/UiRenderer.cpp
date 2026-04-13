#include "UiRenderer.h"
#include "UiActions.h"

#include <imgui.h>

namespace
{
using namespace airchestra::theme;

void renderPanelCard(const char* label,
                     const char* description,
                     bool isActive,
                     airchestra::DetailPanel panel,
                     airchestra::ViewState& state,
                     airchestra::EventLogger& logger)
{
    const auto isSelected = state.selectedPanel == panel;

    ImGui::PushID(airchestra::toDisplayName(panel));

    if (isSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, kBgFrame);
        ImGui::PushStyleColor(ImGuiCol_Border, kBorderAccent);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, kBgPanel);
        ImGui::PushStyleColor(ImGuiCol_Border, kBorder);
    }

    const float cardWidth = juce::jmax(140.0f, ImGui::GetContentRegionAvail().x);
    const float cardHeight = 110.0f;

    ImGui::BeginChild("##card", ImVec2(cardWidth, cardHeight), ImGuiChildFlags_Borders);

    auto* drawList = ImGui::GetWindowDrawList();
    const auto topLeft = ImGui::GetCursorScreenPos();

    // Colored panel icon (small rounded square)
    drawList->AddRectFilled(
        ImVec2(topLeft.x, topLeft.y + 2.0f),
        ImVec2(topLeft.x + 12.0f, topLeft.y + 14.0f),
        panelDotColor(panel), 3.0f);

    // Status dot (top-right)
    drawList->AddCircleFilled(
        ImVec2(topLeft.x + cardWidth - 32.0f, topLeft.y + 8.0f),
        4.0f,
        isActive ? ImGui::ColorConvertFloat4ToU32(kGreen) : ImGui::ColorConvertFloat4ToU32(kTextDim));

    ImGui::Dummy(ImVec2(18.0f, 0.0f));
    ImGui::SameLine();

    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.1f);
    ImGui::TextUnformatted(label);
    ImGui::PopFont();

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, kTextSecondary);
    ImGui::TextWrapped("%s", description);
    ImGui::PopStyleColor();

    ImGui::EndChild();

    // Hover effect and click handling on the whole card
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        auto* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                          ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.04f)), 6.0f);
    }
    if (ImGui::IsItemClicked())
        airchestra::ui_actions::selectPanel(state, logger, panel, "control_room_card");

    ImGui::PopStyleColor(2);
    ImGui::PopID();
}

void renderPanelDetails(airchestra::ViewState& state, airchestra::EventLogger& logger)
{
    auto* drawList = ImGui::GetWindowDrawList();
    auto pos = ImGui::GetCursorScreenPos();
    drawList->AddRectFilled(
        ImVec2(pos.x, pos.y + 2.0f),
        ImVec2(pos.x + 12.0f, pos.y + 14.0f),
        panelDotColor(state.selectedPanel), 3.0f);
    ImGui::Dummy(ImVec2(18.0f, 0.0f));
    ImGui::SameLine();

    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.15f);
    ImGui::Text("%s", airchestra::toDisplayName(state.selectedPanel));
    ImGui::PopFont();

    ImGui::Separator();
    ImGui::Spacing();

    switch (state.selectedPanel)
    {
        case airchestra::DetailPanel::Camera:
        {
            ImGui::PushStyleColor(ImGuiCol_Text, kTextSecondary);
            ImGui::BulletText("Status: Not connected");
            ImGui::BulletText("Tracking: No data");
            ImGui::PopStyleColor();
            ImGui::Spacing();

            auto enabled = state.cameraPreviewEnabled;
            if (ImGui::Checkbox("Enable mock tracking preview", &enabled))
            {
                state.cameraPreviewEnabled = enabled;
                airchestra::ui_actions::recordSettingChanged(state, logger, "camera_preview_enabled", enabled ? "true" : "false");
            }

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, kAccent);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kAccentHover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, kAccentActive);
            if (ImGui::Button("Open Camera Settings", ImVec2(200.0f, 32.0f)))
            {
                airchestra::ui_actions::recordButtonClick(state, logger, "open_camera_settings", "Opening settings for camera placeholders");
                airchestra::ui_actions::changeScreen(state, logger, airchestra::AppScreen::Settings, "camera_panel");
            }
            ImGui::PopStyleColor(3);
            break;
        }

        case airchestra::DetailPanel::Audio:
        {
            ImGui::PushStyleColor(ImGuiCol_Text, kTextSecondary);
            ImGui::BulletText("Audio: Not initialized");
            ImGui::BulletText("MIDI: Not initialized");
            ImGui::PopStyleColor();
            ImGui::Spacing();

            auto audioArmed = state.audioPlaceholderArmed;
            if (ImGui::Checkbox("Arm audio placeholder", &audioArmed))
            {
                state.audioPlaceholderArmed = audioArmed;
                airchestra::ui_actions::recordSettingChanged(state, logger, "audio_placeholder_armed", audioArmed ? "true" : "false");
            }

            auto midiArmed = state.midiPlaceholderArmed;
            if (ImGui::Checkbox("Arm MIDI placeholder", &midiArmed))
            {
                state.midiPlaceholderArmed = midiArmed;
                airchestra::ui_actions::recordSettingChanged(state, logger, "midi_placeholder_armed", midiArmed ? "true" : "false");
            }
            break;
        }

        case airchestra::DetailPanel::UI:
        {
            ImGui::PushStyleColor(ImGuiCol_Text, kTextSecondary);
            ImGui::BulletText("Navigation: Landing / Control Room / Settings / About");
            ImGui::BulletText("Overlay: %s", state.debugOverlayVisible ? "Visible" : "Hidden");
            ImGui::PopStyleColor();
            ImGui::Spacing();

            if (ImGui::Button(state.debugOverlayVisible ? "Hide Debug Overlay" : "Show Debug Overlay", ImVec2(200.0f, 32.0f)))
                airchestra::ui_actions::setOverlayVisible(state, logger, !state.debugOverlayVisible, "ui_panel");

            auto highContrast = state.highContrastUi;
            if (ImGui::Checkbox("High contrast UI", &highContrast))
            {
                state.highContrastUi = highContrast;
                airchestra::ui_actions::recordSettingChanged(state, logger, "high_contrast_ui", highContrast ? "true" : "false");
            }
            break;
        }

        case airchestra::DetailPanel::SystemState:
        {
            ImGui::PushStyleColor(ImGuiCol_Text, kTextSecondary);
            ImGui::BulletText("Session: %s", state.sessionRunning ? "Running" : "Stopped");
            ImGui::BulletText("Mock sensitivity: %.2f", static_cast<double>(state.mockSensitivity));
            ImGui::PopStyleColor();
            ImGui::Spacing();

            const bool running = state.sessionRunning;
            if (running)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, kOrange);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.64f, 0.27f, 1.0f));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, kGreen);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.80f, 0.55f, 1.0f));
            }
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            if (ImGui::Button(running ? "Stop Session" : "Start Session", ImVec2(160.0f, 32.0f)))
                airchestra::ui_actions::setSessionRunning(state, logger, !running, "system_panel");
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::Button("Reset UI State", ImVec2(160.0f, 32.0f)))
            {
                state.sessionRunning = false;
                state.debugOverlayVisible = true;
                state.compactOverlay = false;
                state.showLogPathInOverlay = true;
                state.cameraPreviewEnabled = false;
                state.audioPlaceholderArmed = false;
                state.midiPlaceholderArmed = false;
                state.selectedPanel = airchestra::DetailPanel::SystemState;
                airchestra::ui_actions::recordButtonClick(state, logger, "reset_ui_state", "UI state reset");
            }
            break;
        }

        case airchestra::DetailPanel::InteractionLog:
        {
            ImGui::PushStyleColor(ImGuiCol_Text, kTextSecondary);
            ImGui::BulletText("Count: %d", state.interactionCount);
            ImGui::BulletText("Last: %s", state.lastInteraction.toRawUTF8());
            ImGui::BulletText("%s", logger.getStatusText().toRawUTF8());
            ImGui::PopStyleColor();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, kTextDim);
            ImGui::TextWrapped("%s", logger.getLogFile().getFullPathName().toRawUTF8());
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, kAccent);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kAccentHover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, kAccentActive);
            if (ImGui::Button("Reveal Log Folder", ImVec2(180.0f, 32.0f)))
            {
                logger.getLogFile().getParentDirectory().revealToUser();
                airchestra::ui_actions::recordButtonClick(state, logger, "reveal_log_folder", "Opened log folder");
            }
            ImGui::PopStyleColor(3);
            break;
        }
    }
}

void renderTopBar(airchestra::ViewState& state, airchestra::EventLogger& logger)
{
    // App branding
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, kAccent);
    ImGui::TextUnformatted("Airchestra");
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::SameLine();
    const float subtitleY = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(subtitleY + 6.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, kTextDim);
    ImGui::TextUnformatted("Gesture-Controlled MIDI Performance");
    ImGui::PopStyleColor();
    ImGui::SetCursorPosY(subtitleY + ImGui::GetTextLineHeightWithSpacing());

    ImGui::Spacing();

    // Tab-based navigation
    if (ImGui::BeginTabBar("NavTabs", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton))
    {
        auto navTab = [&](const char* label, airchestra::AppScreen target)
        {
            ImGuiTabItemFlags flags = 0;
            if (state.currentScreen == target)
                flags |= ImGuiTabItemFlags_SetSelected;

            if (ImGui::BeginTabItem(label, nullptr, flags))
            {
                if (state.currentScreen != target)
                {
                    airchestra::ui_actions::recordButtonClick(state, logger, label,
                        juce::String("Opening ") + airchestra::toDisplayName(target));
                    airchestra::ui_actions::changeScreen(state, logger, target, label);
                }
                ImGui::EndTabItem();
            }
        };

        navTab("Home",         airchestra::AppScreen::Landing);
        navTab("Control Room", airchestra::AppScreen::Session);
        navTab("Settings",     airchestra::AppScreen::Settings);
        navTab("About",        airchestra::AppScreen::About);

        // Right-aligned overlay toggle
        const float avail = ImGui::GetContentRegionAvail().x;
        if (avail > 130.0f)
        {
            ImGui::SameLine(ImGui::GetCursorPosX() + avail - 125.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.08f));
            ImGui::PushStyleColor(ImGuiCol_Text, kTextSecondary);
            if (ImGui::Button(state.debugOverlayVisible ? "Hide Overlay" : "Show Overlay", ImVec2(115.0f, 0.0f)))
                airchestra::ui_actions::setOverlayVisible(state, logger, !state.debugOverlayVisible, "top_bar");
            ImGui::PopStyleColor(3);
        }

        ImGui::EndTabBar();
    }
}

void renderControlRoom(airchestra::ViewState& state, airchestra::EventLogger& logger)
{
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.4f);
    ImGui::TextUnformatted("Control Room");
    ImGui::PopFont();

    ImGui::PushStyleColor(ImGuiCol_Text, kTextSecondary);
    ImGui::TextWrapped("Clickable Week 1 shell for Camera, Audio, UI, System State, and Interaction Log. Camera/audio/MIDI not yet initialized.");
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    // Session control
    {
        const bool running = state.sessionRunning;
        if (running)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, kOrange);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.64f, 0.27f, 1.0f));
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, kGreen);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.80f, 0.55f, 1.0f));
        }
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        if (ImGui::Button(running ? "  Stop Session  " : "  Start Session  ", ImVec2(180.0f, 40.0f)))
            airchestra::ui_actions::setSessionRunning(state, logger, !running, "control_room");

        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        auto* dl = ImGui::GetWindowDrawList();
        auto dotPos = ImGui::GetCursorScreenPos();
        dotPos.y += 14.0f;
        dl->AddCircleFilled(dotPos, 5.0f,
            ImGui::ColorConvertFloat4ToU32(running ? kGreen : kTextDim));
        ImGui::Dummy(ImVec2(14.0f, 0.0f));
        ImGui::SameLine();
        ImGui::Text("Session: %s", running ? "Running" : "Stopped");
    }

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    ImGui::SeparatorText("Panels");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    // Panel cards grid
    if (ImGui::BeginTable("Panel Cards", 5, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_PadOuterX))
    {
        ImGui::TableNextColumn();
        renderPanelCard("Camera", "Webcam and hand landmark placeholder.",
            state.cameraPreviewEnabled, airchestra::DetailPanel::Camera, state, logger);

        ImGui::TableNextColumn();
        renderPanelCard("Audio", "Audio and MIDI placeholder.",
            state.audioPlaceholderArmed || state.midiPlaceholderArmed,
            airchestra::DetailPanel::Audio, state, logger);

        ImGui::TableNextColumn();
        renderPanelCard("UI", "Navigation and overlay controls.",
            state.debugOverlayVisible, airchestra::DetailPanel::UI, state, logger);

        ImGui::TableNextColumn();
        renderPanelCard("System", "Runtime state and reset tools.",
            state.sessionRunning, airchestra::DetailPanel::SystemState, state, logger);

        ImGui::TableNextColumn();
        renderPanelCard("Log", "Local JSONL events.",
            true, airchestra::DetailPanel::InteractionLog, state, logger);

        ImGui::EndTable();
    }

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    ImGui::SeparatorText("Details");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, kBgPanel);
    ImGui::BeginChild("##details", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
    renderPanelDetails(state, logger);
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void renderSettings(airchestra::ViewState& state, airchestra::EventLogger& logger)
{
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.4f);
    ImGui::TextUnformatted("Settings");
    ImGui::PopFont();

    ImGui::PushStyleColor(ImGuiCol_Text, kTextSecondary);
    ImGui::TextWrapped("Local UI/debug controls. Safe to replace with real Camera/Audio/MIDI settings later.");
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0.0f, 12.0f));

    // Overlay section
    ImGui::SeparatorText("Overlay");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    ImGui::Indent(8.0f);

    auto overlayVisible = state.debugOverlayVisible;
    if (ImGui::Checkbox("Show debug overlay", &overlayVisible))
        airchestra::ui_actions::setOverlayVisible(state, logger, overlayVisible, "settings");

    auto compactOverlay = state.compactOverlay;
    if (ImGui::Checkbox("Compact overlay", &compactOverlay))
    {
        state.compactOverlay = compactOverlay;
        airchestra::ui_actions::recordSettingChanged(state, logger, "compact_overlay", compactOverlay ? "true" : "false");
    }

    auto showLogPath = state.showLogPathInOverlay;
    if (ImGui::Checkbox("Show log path in overlay", &showLogPath))
    {
        state.showLogPathInOverlay = showLogPath;
        airchestra::ui_actions::recordSettingChanged(state, logger, "show_log_path_in_overlay", showLogPath ? "true" : "false");
    }

    ImGui::Unindent(8.0f);
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    // Input section
    ImGui::SeparatorText("Input");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    ImGui::Indent(8.0f);

    auto simulatePreview = state.simulateInputPreview;
    if (ImGui::Checkbox("Enable mock input preview", &simulatePreview))
    {
        state.simulateInputPreview = simulatePreview;
        airchestra::ui_actions::recordSettingChanged(state, logger, "simulate_input_preview", simulatePreview ? "true" : "false");
    }

    ImGui::SetNextItemWidth(300.0f);
    auto sensitivity = state.mockSensitivity;
    if (ImGui::SliderFloat("Mock sensitivity", &sensitivity, 0.10f, 1.00f, "%.2f"))
        state.mockSensitivity = sensitivity;

    if (ImGui::IsItemDeactivatedAfterEdit())
        airchestra::ui_actions::recordSettingChanged(state, logger, "mock_sensitivity", juce::String(state.mockSensitivity, 2));

    ImGui::Unindent(8.0f);
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    // Appearance section
    ImGui::SeparatorText("Appearance");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    ImGui::Indent(8.0f);

    auto highContrast = state.highContrastUi;
    if (ImGui::Checkbox("High contrast UI", &highContrast))
    {
        state.highContrastUi = highContrast;
        airchestra::ui_actions::recordSettingChanged(state, logger, "high_contrast_ui", highContrast ? "true" : "false");
    }

    ImGui::Unindent(8.0f);
    ImGui::Dummy(ImVec2(0.0f, 16.0f));

    // Ghost-style back button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.06f));
    ImGui::PushStyleColor(ImGuiCol_Text, kAccent);
    if (ImGui::Button("<  Back to Control Room", ImVec2(240.0f, 36.0f)))
    {
        airchestra::ui_actions::recordButtonClick(state, logger, "back_to_control_room", "Opening Control Room");
        airchestra::ui_actions::changeScreen(state, logger, airchestra::AppScreen::Session, "settings");
    }
    ImGui::PopStyleColor(3);
}

void renderAbout(airchestra::ViewState& state, airchestra::EventLogger& logger)
{
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.4f);
    ImGui::TextUnformatted("About");
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    ImGui::PushStyleColor(ImGuiCol_Text, kTextSecondary);
    ImGui::TextWrapped("Airchestra is a KTH II1305 gesture-controlled music prototype. This app shell focuses on the UX/debug layer: windowing, ImGui rendering, clickable placeholders, and local event logging.");
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, kTextDim);
    ImGui::BulletText("Camera, MediaPipe, audio synthesis, and MIDI output are intentionally not initialized in this task.");
    ImGui::BulletText("Logs are stored as JSONL next to the executable.");
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0.0f, 16.0f));

    // Ghost back button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.06f));
    ImGui::PushStyleColor(ImGuiCol_Text, kAccent);
    if (ImGui::Button("<  Back to Home", ImVec2(180.0f, 36.0f)))
    {
        airchestra::ui_actions::recordButtonClick(state, logger, "back_to_home", "Opening Home");
        airchestra::ui_actions::changeScreen(state, logger, airchestra::AppScreen::Landing, "about");
    }
    ImGui::PopStyleColor(3);
}
}

namespace airchestra
{
UiRenderer::UiRenderer(ViewState& viewState, EventLogger& eventLogger)
    : state(viewState),
      logger(eventLogger)
{
}

void UiRenderer::render()
{
    const auto displaySize = ImGui::GetIO().DisplaySize;
    auto pushedStyleColours = 0;

    if (state.highContrastUi)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.04f, 0.05f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.40f, 0.60f, 1.0f));
        pushedStyleColours = 3;
    }

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(displaySize, ImGuiCond_Always);

    constexpr auto rootFlags = ImGuiWindowFlags_NoDecoration
                             | ImGuiWindowFlags_NoMove
                             | ImGuiWindowFlags_NoResize
                             | ImGuiWindowFlags_NoSavedSettings
                             | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("Airchestra Root", nullptr, rootFlags);

    renderTopBar(state, logger);

    ImGui::Spacing();

    if (state.currentScreen == AppScreen::Landing && !state.landingPageShownLogged)
    {
        state.landingPageShownLogged = true;
        logger.log(AppEventType::LandingPageShown, { { "view", "landing" } });
    }

    const auto contentHeight = juce::jmax(360.0f, displaySize.y - 160.0f);
    ImGui::BeginChild("Main Content", ImVec2(displaySize.x - 32.0f, contentHeight), ImGuiChildFlags_Borders);

    switch (state.currentScreen)
    {
        case AppScreen::Landing:
            landingPage.render(state, logger);
            break;

        case AppScreen::Session:
            renderControlRoom(state, logger);
            break;

        case AppScreen::Settings:
            renderSettings(state, logger);
            break;

        case AppScreen::About:
            renderAbout(state, logger);
            break;
    }

    ImGui::EndChild();

    // Status footer
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, theme::kTextDim);
    ImGui::Text("Airchestra v0.1.0");
    ImGui::SameLine(displaySize.x - 300.0f);
    ImGui::Text("Events: %d  |  %s", state.interactionCount, state.appStatus.toRawUTF8());
    ImGui::PopStyleColor();

    ImGui::End();

    if (state.debugOverlayVisible)
        overlay.render(state, logger);

    if (pushedStyleColours > 0)
        ImGui::PopStyleColor(pushedStyleColours);
}
}
