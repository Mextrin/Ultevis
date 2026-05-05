#include "AudioEngine.h"
#include <iostream>

namespace
{
    // Translates custom SM Drums notes back into standard General MIDI
    int translateSMtoGM(int smNote)
    {
        switch (smNote)
        {
            case 43: return 41; // Floor Tom: translates SM 43 to GM 41
            case 48: return 47; // High Tom: translates SM 50 to GM 48
            case 54: return 49; // Crash Cymbal: translates SM 54 to GM 49
            case 60: return 51; // Ride Cymbal: translates SM 60 to GM 51

            // Kick (36), Snare (38), and Hi-Hats (42, 46), Low Tom (45) are already GM standard
            default: return smNote;
        }
    }

    int midiVelocityFromPercent(int percent)
    {
        const int p = juce::jlimit(0, 100, percent);
        return juce::jlimit(0, 127, static_cast<int>((static_cast<long long>(127) * p + 50) / 100));
    }
}

// Resets drum playback/trigger state so stale hits are not consumed
void HeadlessAudioEngine::resetDrumPlaybackState()
{
    if (globalState == nullptr)
        return;

    globalState->leftDrumHit.store(false);
    globalState->rightDrumHit.store(false);
    globalState->mouthKickHit.store(false);
    globalState->leftDrumType.store(36);
    globalState->rightDrumType.store(38);
}

// Loads drum SFZ instrument into drumSynth
void HeadlessAudioEngine::loadDrumSound(const juce::String& sfzPath)
{
    std::lock_guard<std::mutex> lock(drumSynthMutex);

    // Avoid reloading the same drum kit repeatedly
    if (loadedDrumSfzPath == sfzPath) {
        std::cout << "Drum SFZ already loaded, skipping reload: "
                  << sfzPath << std::endl;
        resetDrumPlaybackState();
        return;
    }

    resetDrumPlaybackState();

    std::cout << "Loading drum SFZ: " << sfzPath << std::endl;

    bool loaded = drumSynth.loadSfzFile(sfzPath.toStdString());
    if (!loaded) {
        std::cerr << "ERROR: Failed to load SFZ: " << sfzPath << std::endl;
        return;
    }

    loadedDrumSfzPath = sfzPath;
}

// Processes the drums
void HeadlessAudioEngine::processDrums(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (buffer.getNumChannels() < 2)
        return;

    constexpr int mouthKickNote = 36;
    constexpr int mouthKickVelocity = 110;

    std::lock_guard<std::mutex> lock(drumSynthMutex);

    if (globalState->leftDrumHit.exchange(false)) {
        const int leftNote = globalState->leftDrumType.load();
        const int leftVelocity = midiVelocityFromPercent(globalState->leftDrumVelocity.load());
        if (leftVelocity > 0) {
            int standardLeftGMNote = translateSMtoGM(leftNote);

            drumSynth.noteOn(0, leftNote, leftVelocity);
            if (midiOut != nullptr)
                midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, standardLeftGMNote, (juce::uint8)leftVelocity));
        }
    }

    if (globalState->rightDrumHit.exchange(false)) {
        const int rightNote = globalState->rightDrumType.load();
        const int rightVelocity = midiVelocityFromPercent(globalState->rightDrumVelocity.load());
        if (rightVelocity > 0) {
            int standardRightGMNote = translateSMtoGM(rightNote);

            drumSynth.noteOn(0, rightNote, rightVelocity);
            if (midiOut != nullptr)
                midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, standardRightGMNote, (juce::uint8)rightVelocity));
        }
    }

    if (globalState->mouthKickHit.exchange(false) && globalState->mouthKickEnable.load()) {
        drumSynth.noteOn(0, mouthKickNote, mouthKickVelocity);
        if (midiOut != nullptr)
            midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, mouthKickNote, (juce::uint8)mouthKickVelocity));
    }

    float* outChannels[] = { buffer.getWritePointer(0), buffer.getWritePointer(1) };
    drumSynth.renderBlock(outChannels, numSamples);
}
