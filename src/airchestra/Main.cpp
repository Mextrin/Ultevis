#include "EventLogger.h"
#include "MainComponent.h"
#include "UiActions.h"
#include "ViewState.h"

#include <juce_gui_extra/juce_gui_extra.h>

namespace
{
int runSmokeTest(airchestra::EventLogger& logger)
{
    logger.log(airchestra::AppEventType::ButtonClicked,
               { { "button", "smoke_test" },
                 { "status", "Airchestra smoke test executed" } });

    airchestra::ViewState state;
    airchestra::ui_actions::recordButtonClick(state, logger, "smoke_start", "Smoke test starting session");
    airchestra::ui_actions::setSessionRunning(state, logger, true, "smoke_test");
    airchestra::ui_actions::changeScreen(state, logger, airchestra::AppScreen::Session, "smoke_test");
    airchestra::ui_actions::selectPanel(state, logger, airchestra::DetailPanel::Camera, "smoke_test");
    state.highContrastUi = true;
    airchestra::ui_actions::recordSettingChanged(state, logger, "high_contrast_ui", "true");
    airchestra::ui_actions::setOverlayVisible(state, logger, false, "smoke_test");

    return 0;
}
}

class AirchestraApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& commandLine) override
    {
        const auto isSmokeTest = commandLine.contains("--smoke-test");
        const auto autoStartSession = commandLine.contains("--autostart-session");

        logger.log(airchestra::AppEventType::AppStarted,
                   { { "mode", isSmokeTest ? "smoke-test" : "interactive" },
                     { "autostart_session", autoStartSession ? "true" : "false" } });

        if (isSmokeTest)
        {
            setApplicationReturnValue(runSmokeTest(logger));
            quit();
            return;
        }

        mainWindow = std::make_unique<MainWindow>(getApplicationName(), logger, autoStartSession);
        logger.log(airchestra::AppEventType::MainWindowCreated,
                   { { "title", getApplicationName() },
                     { "size", "1280x800" } });
    }

    void shutdown() override
    {
        logger.log(airchestra::AppEventType::AppClosing);
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
    }

private:
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name, airchestra::EventLogger& logger, bool autoStartSession)
            : DocumentWindow(name,
                             juce::Desktop::getInstance().getDefaultLookAndFeel()
                                 .findColour(juce::ResizableWindow::backgroundColourId),
                             juce::DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new airchestra::MainComponent(logger, autoStartSession), true);
            setResizable(true, true);
            setResizeLimits(960, 600, 2560, 1600);
            centreWithSize(1280, 800);
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

    airchestra::EventLogger logger;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(AirchestraApplication)
