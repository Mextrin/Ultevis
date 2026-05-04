#include "AudioEngine.h"
#include "GuitarChords.h"
#include <iostream>

void HeadlessAudioEngine::resetGuitarPlaybackState()
{
    if (globalState == nullptr)
        return;

    globalState->guitarStrumHit.store(false);
    globalState->guitarVelocity.store(100);
    guitarChordActive = false;
}

void HeadlessAudioEngine::loadGuitarSound(int guitarSoundID)
{
    juce::String sfzToLoad;

    if (guitarSoundID == 0) {
        sfzToLoad = "Instruments/EGuitarFSBS-bridge-clean-SFZ-20220911/EGuitarFSBS-bridge-clean-20220911.sfz";
    }
    else if (guitarSoundID == 1) {
        sfzToLoad = "Instruments/EGuitarFSBS-bridge-dist1-SFZ-20220911/EGuitarFSBS-bridge-dist1-20220911.sfz";
    }
    else {
        std::cerr << "ERROR: Invalid guitar sound ID: "
                  << guitarSoundID << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(guitarSynthMutex);

    if (loadedGuitarSoundID == guitarSoundID) {
        std::cout << "Guitar SFZ already loaded, skipping reload: "
                  << sfzToLoad << std::endl;
        resetGuitarPlaybackState();
        return;
    }

    resetGuitarPlaybackState();

    std::cout << "Loading guitar SFZ: " << sfzToLoad << std::endl;

    juce::File sfzFile(sfzToLoad);

    bool loaded = guitarSynth.loadSfzFile(sfzFile.getFullPathName().toStdString());

    if (!loaded) {
        std::cerr << "ERROR: Failed to load guitar SFZ: "
                  << sfzToLoad << std::endl;
        return;
    }

    loadedGuitarSoundID = guitarSoundID;
}

void HeadlessAudioEngine::processGuitar(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (buffer.getNumChannels() < 2)
        return;

    std::lock_guard<std::mutex> lock(guitarSynthMutex);

    if (globalState->guitarStrumHit.exchange(false)) {
        const int* chord = airchestra::getGuitarChord(
            globalState->currentGuitarRoot.load(),
            globalState->currentGuitarQuality.load()
        );

        const int velocity = globalState->guitarVelocity.load();

        for (int i = 0; i < 6; ++i) {
            const int note = chord[i];
            if (note < 0)
                continue;

            guitarSynth.noteOn(0, note, velocity);

            if (midiOut != nullptr) {
                midiOut->sendMessageNow(
                    juce::MidiMessage::noteOn(1, note, (juce::uint8)velocity)
                );
            }
        }

        guitarChordActive = true;
    }

    float* outChannels[] = {
        buffer.getWritePointer(0),
        buffer.getWritePointer(1)
    };

    guitarSynth.renderBlock(outChannels, numSamples);
}