#include "MainComponent.h"

#include "../theremin/GestureMappings.h"

namespace airchestra
{
MainComponent::MainComponent(EventLogger& eventLogger)
    : logger(eventLogger),
      thereminAudio(handState),
      mockInput(handState),
      midiOutput(handState),
      uiRenderer(viewState, logger),
      imguiLayer(uiRenderer, logger)
{
    mockInput.setEnabled(false);
    mockInput.setMotionScale(viewState.mockSensitivity);
    updateThereminReadout();
    lastMidiStatus = viewState.midiStatus;
    lastMidiOpen = viewState.midiOpen;

    addAndMakeVisible(imguiLayer);
    setSize(1280, 800);
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    stopTimer();
}

void MainComponent::resized()
{
    imguiLayer.setBounds(getLocalBounds());
}

void MainComponent::timerCallback()
{
    mockInput.setMotionScale(viewState.mockSensitivity);
    mockInput.setEnabled(viewState.sessionRunning && viewState.simulateInputPreview);
    updateThereminReadout();
}

void MainComponent::updateThereminReadout()
{
    const auto snapshot = handState.snapshot();
    const auto midiStatus = midiOutput.getStatusText();
    const auto midiOpen = midiOutput.isOpen();
    const auto midiDeviceName = midiOutput.getOpenedDeviceName();

    viewState.handX = snapshot.x;
    viewState.handY = snapshot.y;
    viewState.handActive = snapshot.active;
    viewState.mockInputActive = mockInput.isEnabled();
    viewState.frequencyHz = ii1305::GestureMappings::normalizedXToFrequencyHz(snapshot.x);
    viewState.pitchBend = ii1305::GestureMappings::normalizedXToPitchBend(snapshot.x);
    viewState.expression = snapshot.active ? ii1305::GestureMappings::normalizedYToExpression(snapshot.y) : 0;
    viewState.audioRunning = true;
    viewState.midiOpen = midiOpen;
    viewState.midiDeviceName = midiDeviceName;
    viewState.midiStatus = midiStatus;

    if (midiStatus != lastMidiStatus || midiOpen != lastMidiOpen)
    {
        logger.log(AppEventType::MidiStatusChanged,
                   { { "open", midiOpen ? "true" : "false" },
                     { "device", midiDeviceName.isNotEmpty() ? midiDeviceName : "none" },
                     { "status", midiStatus } });
        lastMidiStatus = midiStatus;
        lastMidiOpen = midiOpen;
    }
}
}
