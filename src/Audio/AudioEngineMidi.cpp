#include "AudioEngine.h"

// Returns whether MIDI output is currently active.
bool HeadlessAudioEngine::isMidiEnabled() const
{
    return midiOut != nullptr;
}

std::vector<std::pair<std::string, std::string>> HeadlessAudioEngine::getAvailableMidiDevices() const
{
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& d : juce::MidiOutput::getAvailableDevices())
        result.push_back({ d.identifier.toStdString(), d.name.toStdString() });
    return result;
}

void HeadlessAudioEngine::openMidiDevice(const std::string& identifier)
{
    if (identifier.empty()) {
        // If "None" is selected, close the port by resetting the pointer
        midiOut.reset();
    } else {
        // Ask JUCE to physically open the port to macOS (like your IAC Bus)
        midiOut = juce::MidiOutput::openDevice(identifier);
    }
}