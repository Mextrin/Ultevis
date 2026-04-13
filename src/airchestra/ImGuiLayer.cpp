#include "ImGuiLayer.h"
#include "ViewState.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>

namespace
{
void applyAirchestraTheme()
{
    using namespace airchestra::theme;

    auto& style = ImGui::GetStyle();

    style.WindowPadding     = ImVec2(16.0f, 16.0f);
    style.FramePadding      = ImVec2(12.0f, 8.0f);
    style.CellPadding       = ImVec2(10.0f, 8.0f);
    style.ItemSpacing       = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 10.0f;

    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 6.0f;

    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.TabBorderSize     = 0.0f;
    style.TabBarBorderSize  = 1.0f;
    style.TabBarOverlineSize = 2.0f;
    style.SeparatorTextBorderSize = 1.0f;

    auto* c = style.Colors;
    c[ImGuiCol_Text]                  = kTextPrimary;
    c[ImGuiCol_TextDisabled]          = kTextSecondary;
    c[ImGuiCol_WindowBg]              = kBgDark;
    c[ImGuiCol_ChildBg]               = kBgChild;
    c[ImGuiCol_PopupBg]               = kBgPanel;
    c[ImGuiCol_Border]                = kBorder;
    c[ImGuiCol_BorderShadow]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_FrameBg]               = kBgFrame;
    c[ImGuiCol_FrameBgHovered]        = ImVec4(0.173f, 0.188f, 0.224f, 1.0f);
    c[ImGuiCol_FrameBgActive]         = ImVec4(0.204f, 0.220f, 0.259f, 1.0f);
    c[ImGuiCol_TitleBg]               = kBgPanel;
    c[ImGuiCol_TitleBgActive]         = kBgPanel;
    c[ImGuiCol_TitleBgCollapsed]      = kBgDark;
    c[ImGuiCol_MenuBarBg]             = kBgPanel;
    c[ImGuiCol_ScrollbarBg]           = kBgDark;
    c[ImGuiCol_ScrollbarGrab]         = ImVec4(0.24f, 0.26f, 0.30f, 1.0f);
    c[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.30f, 0.33f, 0.38f, 1.0f);
    c[ImGuiCol_ScrollbarGrabActive]   = kAccent;
    c[ImGuiCol_CheckMark]             = kAccent;
    c[ImGuiCol_SliderGrab]            = kAccent;
    c[ImGuiCol_SliderGrabActive]      = kAccentHover;
    c[ImGuiCol_Button]                = ImVec4(0.173f, 0.188f, 0.224f, 1.0f);
    c[ImGuiCol_ButtonHovered]         = ImVec4(0.220f, 0.240f, 0.290f, 1.0f);
    c[ImGuiCol_ButtonActive]          = kAccentActive;
    c[ImGuiCol_Header]                = ImVec4(0.173f, 0.188f, 0.224f, 0.60f);
    c[ImGuiCol_HeaderHovered]         = ImVec4(0.220f, 0.240f, 0.290f, 0.80f);
    c[ImGuiCol_HeaderActive]          = kAccent;
    c[ImGuiCol_Separator]             = kBorder;
    c[ImGuiCol_SeparatorHovered]      = kAccent;
    c[ImGuiCol_SeparatorActive]       = kAccentActive;
    c[ImGuiCol_Tab]                   = kBgFrame;
    c[ImGuiCol_TabHovered]            = ImVec4(0.220f, 0.240f, 0.290f, 1.0f);
    c[ImGuiCol_TabSelected]           = kAccentActive;
    c[ImGuiCol_TabSelectedOverline]   = kAccent;
    c[ImGuiCol_TabDimmed]             = kBgPanel;
    c[ImGuiCol_TabDimmedSelected]     = kBgFrame;
    c[ImGuiCol_TableHeaderBg]         = kBgPanel;
    c[ImGuiCol_TableBorderStrong]     = kBorder;
    c[ImGuiCol_TableBorderLight]      = ImVec4(0.200f, 0.216f, 0.255f, 0.30f);
    c[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);
    c[ImGuiCol_TextSelectedBg]        = kAccentDim;
    c[ImGuiCol_NavCursor]             = kAccent;
    c[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
}

juce::File findPreferredUiFont()
{
#if JUCE_WINDOWS
    const char* candidates[] =
    {
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf"
    };
#elif JUCE_MAC
    const char* candidates[] =
    {
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/SFNS.ttf"
    };
#else
    const char* candidates[] =
    {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"
    };
#endif

    for (const auto* candidate : candidates)
    {
        juce::File fontFile(candidate);
        if (fontFile.existsAsFile())
            return fontFile;
    }

    return {};
}
}

namespace airchestra
{
ImGuiLayer::ImGuiLayer(UiRenderer& rendererToUse, EventLogger& eventLogger)
    : uiRenderer(rendererToUse),
      logger(eventLogger)
{
    for (auto& button : mouseButtons)
        button.store(false, std::memory_order_relaxed);

    setOpaque(true);
    setWantsKeyboardFocus(true);
    setInterceptsMouseClicks(true, true);

    openGLContext.setRenderer(this);
    openGLContext.setComponentPaintingEnabled(false);
    openGLContext.setContinuousRepainting(true);
    openGLContext.attachTo(*this);
}

ImGuiLayer::~ImGuiLayer()
{
    openGLContext.detach();
}

void ImGuiLayer::resized()
{
    openGLContext.triggerRepaint();
}

void ImGuiLayer::mouseMove(const juce::MouseEvent& event)
{
    updateMousePosition(event);
}

void ImGuiLayer::mouseDrag(const juce::MouseEvent& event)
{
    updateMousePosition(event);
    updateMouseButtons(event);
}

void ImGuiLayer::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    updateMousePosition(event);
    updateMouseButtons(event);
}

void ImGuiLayer::mouseUp(const juce::MouseEvent& event)
{
    updateMousePosition(event);
    updateMouseButtons(event);
}

void ImGuiLayer::mouseExit(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    mouseX.store(-1.0f, std::memory_order_relaxed);
    mouseY.store(-1.0f, std::memory_order_relaxed);
}

void ImGuiLayer::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    updateMousePosition(event);
    mouseWheelX.fetch_add(wheel.deltaX, std::memory_order_relaxed);
    mouseWheelY.fetch_add(wheel.deltaY, std::memory_order_relaxed);
}

void ImGuiLayer::newOpenGLContextCreated()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    applyAirchestraTheme();

    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImFontConfig fontCfg;
    fontCfg.OversampleH = 3;
    fontCfg.OversampleV = 2;
    fontCfg.PixelSnapH = false;

    const auto uiFont = findPreferredUiFont();
    if (uiFont.existsAsFile())
    {
        const auto fontPath = uiFont.getFullPathName();
        io.Fonts->AddFontFromFileTTF(fontPath.toRawUTF8(), 16.0f, &fontCfg);
    }
    else
    {
        fontCfg.SizePixels = 16.0f;
        io.Fonts->AddFontDefault(&fontCfg);
    }

    imguiReady = ImGui_ImplOpenGL3_Init("#version 130");
    lastFrameTimeSeconds = juce::Time::getMillisecondCounterHiRes() * 0.001;

    logger.log(AppEventType::ImGuiInitialized,
               { { "backend", "opengl3" },
                 { "status", imguiReady ? "ok" : "failed" } });
}

void ImGuiLayer::renderOpenGL()
{
    const auto scale = static_cast<float>(openGLContext.getRenderingScale());
    juce::gl::glViewport(0,
                         0,
                         juce::roundToInt(static_cast<float>(getWidth()) * scale),
                         juce::roundToInt(static_cast<float>(getHeight()) * scale));
    juce::OpenGLHelpers::clear(juce::Colour(0xff101216));

    if (!imguiReady)
        return;

    auto& io = ImGui::GetIO();
    const auto nowSeconds = juce::Time::getMillisecondCounterHiRes() * 0.001;
    io.DeltaTime = static_cast<float>(juce::jlimit(1.0 / 240.0, 1.0 / 15.0, nowSeconds - lastFrameTimeSeconds));
    lastFrameTimeSeconds = nowSeconds;
    io.DisplaySize = ImVec2(static_cast<float>(getWidth()), static_cast<float>(getHeight()));
    io.DisplayFramebufferScale = ImVec2(scale, scale);

    applyInputToImGui();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    uiRenderer.render();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::openGLContextClosing()
{
    if (!imguiReady)
        return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    imguiReady = false;
}

void ImGuiLayer::updateMousePosition(const juce::MouseEvent& event) noexcept
{
    mouseX.store(static_cast<float>(event.position.x), std::memory_order_relaxed);
    mouseY.store(static_cast<float>(event.position.y), std::memory_order_relaxed);
}

void ImGuiLayer::updateMouseButtons(const juce::MouseEvent& event) noexcept
{
    mouseButtons[0].store(event.mods.isLeftButtonDown(), std::memory_order_relaxed);
    mouseButtons[1].store(event.mods.isRightButtonDown(), std::memory_order_relaxed);
    mouseButtons[2].store(event.mods.isMiddleButtonDown(), std::memory_order_relaxed);
}

void ImGuiLayer::applyInputToImGui()
{
    auto& io = ImGui::GetIO();
    io.MousePos = ImVec2(mouseX.load(std::memory_order_relaxed), mouseY.load(std::memory_order_relaxed));

    for (int i = 0; i < 5; ++i)
        io.MouseDown[i] = mouseButtons[static_cast<size_t>(i)].load(std::memory_order_relaxed);

    io.MouseWheelH += mouseWheelX.exchange(0.0f, std::memory_order_relaxed);
    io.MouseWheel += mouseWheelY.exchange(0.0f, std::memory_order_relaxed);
}
}
