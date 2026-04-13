#include "GestureMappings.h"
#include "HandControlState.h"
#include "MainComponent.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace
{
int runSmokeTest()
{
    ii1305::HandControlState state;
    state.setFromNormalized(0.5f, 0.75f, true);

    const auto snapshot = state.snapshot();
    const auto frequencyHz = ii1305::GestureMappings::normalizedXToFrequencyHz(snapshot.x);
    const auto pitchBend = ii1305::GestureMappings::normalizedXToPitchBend(snapshot.x);
    const auto expression = ii1305::GestureMappings::normalizedYToExpression(snapshot.y);
    const auto devices = juce::MidiOutput::getAvailableDevices();

    juce::ignoreUnused(frequencyHz, pitchBend, expression, devices);
    return 0;
}
}

class ThereminApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& commandLine) override
    {
        if (commandLine.contains("--smoke-test"))
        {
            setApplicationReturnValue(runSmokeTest());
            quit();
            return;
        }

        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
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
        explicit MainWindow(juce::String name)
            : DocumentWindow(name,
                             juce::Desktop::getInstance().getDefaultLookAndFeel()
                                 .findColour(juce::ResizableWindow::backgroundColourId),
                             juce::DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new ii1305::MainComponent(), true);
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(ThereminApplication)
