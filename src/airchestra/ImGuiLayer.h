#pragma once

#include "EventLogger.h"
#include "UiRenderer.h"

#include <array>
#include <atomic>

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>

namespace airchestra
{
class ImGuiLayer final : public juce::Component,
                         private juce::OpenGLRenderer
{
public:
    ImGuiLayer(UiRenderer& rendererToUse, EventLogger& eventLogger);
    ~ImGuiLayer() override;

    void resized() override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    void updateMousePosition(const juce::MouseEvent& event) noexcept;
    void updateMouseButtons(const juce::MouseEvent& event) noexcept;
    void applyInputToImGui();

    UiRenderer& uiRenderer;
    EventLogger& logger;
    juce::OpenGLContext openGLContext;
    std::array<std::atomic<bool>, 5> mouseButtons;
    std::atomic<float> mouseX { -1.0f };
    std::atomic<float> mouseY { -1.0f };
    std::atomic<float> mouseWheelX { 0.0f };
    std::atomic<float> mouseWheelY { 0.0f };
    double lastFrameTimeSeconds = 0.0;
    bool imguiReady = false;
};
}
