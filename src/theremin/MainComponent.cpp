#include "MainComponent.h"

#include "GestureMappings.h"

namespace ii1305
{
MainComponent::MainComponent()
    : audioEngine(handState),
      mockInput(handState),
      midiOutput(handState)
{
    configureLabels();

    addAndMakeVisible(titleLabel);
    addAndMakeVisible(readoutLabel);
    addAndMakeVisible(midiLabel);
    addAndMakeVisible(instructionLabel);

    setSize(680, 360);
    startTimerHz(30);
    updateReadout();
}

MainComponent::~MainComponent()
{
    stopTimer();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff151719));

    g.setColour(juce::Colour(0xff2ed47a));
    g.drawRect(getLocalBounds().reduced(12), 2);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(28);

    titleLabel.setBounds(area.removeFromTop(46));
    area.removeFromTop(12);
    readoutLabel.setBounds(area.removeFromTop(86));
    area.removeFromTop(12);
    midiLabel.setBounds(area.removeFromTop(58));
    area.removeFromTop(12);
    instructionLabel.setBounds(area);
}

void MainComponent::timerCallback()
{
    updateReadout();
}

void MainComponent::configureLabels()
{
    titleLabel.setText("II1305 Theremin Prototype", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::FontOptions(26.0f, juce::Font::bold));

    for (auto* label : { &readoutLabel, &midiLabel, &instructionLabel })
    {
        label->setJustificationType(juce::Justification::centredLeft);
        label->setColour(juce::Label::textColourId, juce::Colour(0xffe8ecef));
        label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    }

    readoutLabel.setFont(juce::FontOptions(18.0f));
    midiLabel.setFont(juce::FontOptions(16.0f));
    instructionLabel.setFont(juce::FontOptions(15.0f));
    instructionLabel.setText("Mock hand input is running at 60 Hz. Move Ableton MIDI input to your loopMIDI port to inspect pitch bend and CC11 expression. MediaPipe can later call HandControlState::setFromNormalized(x, y, active).",
                             juce::dontSendNotification);
}

void MainComponent::updateReadout()
{
    const auto snapshot = handState.snapshot();
    const auto frequencyHz = GestureMappings::normalizedXToFrequencyHz(snapshot.x);
    const auto pitchBend = GestureMappings::normalizedXToPitchBend(snapshot.x);
    const auto expression = GestureMappings::normalizedYToExpression(snapshot.y);

    readoutLabel.setText("x: " + juce::String(snapshot.x, 3)
                           + "    y: " + juce::String(snapshot.y, 3)
                           + "    frequency: " + juce::String(frequencyHz, 1) + " Hz\n"
                           + "pitch bend: " + juce::String(pitchBend)
                           + " / 16383    CC11 expression: " + juce::String(expression)
                           + " / 127",
                         juce::dontSendNotification);

    midiLabel.setText(midiOutput.getStatusText(), juce::dontSendNotification);
}
}
