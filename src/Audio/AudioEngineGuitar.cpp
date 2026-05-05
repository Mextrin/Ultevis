#include "AudioEngine.h"
#include "GuitarChords.h"
#include <iostream>

namespace
{
    bool looksLikeProjectRoot(const juce::File& directory)
    {
        return directory.isDirectory()
            && directory.getChildFile("CMakeLists.txt").existsAsFile()
            && directory.getChildFile("Instruments").isDirectory()
            && directory.getChildFile("qml").isDirectory();
    }

    juce::File searchUpwardsForProjectRoot(juce::File start)
    {
        if (start.existsAsFile())
            start = start.getParentDirectory();

        while (start.exists()) {
            if (looksLikeProjectRoot(start))
                return start;

            const juce::File parent = start.getParentDirectory();
            if (parent == start)
                break;
            start = parent;
        }

        return {};
    }

    juce::File resolvedProjectFile(const juce::String& relativePath)
    {
        if (juce::File::isAbsolutePath(relativePath))
            return juce::File(relativePath);

        const juce::File cwdRoot = searchUpwardsForProjectRoot(juce::File::getCurrentWorkingDirectory());
        if (cwdRoot.isDirectory()) {
            const juce::File candidate = cwdRoot.getChildFile(relativePath);
            if (candidate.exists())
                return candidate;
        }

        const juce::File appRoot = searchUpwardsForProjectRoot(
            juce::File::getSpecialLocation(juce::File::currentApplicationFile)
        );
        if (appRoot.isDirectory()) {
            const juce::File candidate = appRoot.getChildFile(relativePath);
            if (candidate.exists())
                return candidate;
        }

        return juce::File(relativePath);
    }
}

void HeadlessAudioEngine::resetGuitarPlaybackState()
{
    if (globalState == nullptr)
        return;

    globalState->guitarStrumHit.store(false);
    globalState->guitarStrumDirection.store(GuitarStrumDirection::Down);
    globalState->guitarVelocity.store(100);
    guitarChordActive = false;
}

void HeadlessAudioEngine::loadGuitarSound(int guitarSoundID)
{
    juce::String sfzToLoad;

    if (guitarSoundID == 0) {
        sfzToLoad = "Instruments/EGuitarFSBS-bridge-clean-small-SFZ+FLAC-20220911/EGuitarFSBS-bridge-clean-small-20220911.sfz";
    }
    else if (guitarSoundID == 1) {
        sfzToLoad = "Instruments/EGuitarFSBS-bridge-dist2-SFZ+FLAC-20220911/EGuitarFSBS-bridge-dist2-20220911.sfz";
    }
    else if (guitarSoundID == 2) {
        sfzToLoad = "Instruments/FSS-SteelStringGuitar-SFZ-20200521/FSS-SteelStringGuitar-20200521.sfz";
    } else {
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

    const juce::File sfzFile = resolvedProjectFile(sfzToLoad);

    std::cout << "Loading guitar SFZ: " << sfzFile.getFullPathName() << std::endl;

    if (!sfzFile.existsAsFile()) {
        std::cerr << "ERROR: Guitar SFZ not found: "
                  << sfzFile.getFullPathName() << std::endl;
        return;
    }

    bool loaded = guitarSynth.loadSfzFile(sfzFile.getFullPathName().toStdString());

    if (!loaded) {
        std::cerr << "ERROR: Failed to load guitar SFZ: "
                  << sfzFile.getFullPathName() << std::endl;
        return;
    }

    loadedGuitarSfzPath = sfzFile.getFullPathName();
    loadedGuitarSoundID = guitarSoundID;
}

void HeadlessAudioEngine::processGuitar(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (buffer.getNumChannels() < 2)
        return;

    std::lock_guard<std::mutex> lock(guitarSynthMutex);

    static int pendingNotes[6] = { -1, -1, -1, -1, -1, -1 };
    static int pendingVelocity = 100;
    static int nextStringIndex = 0;
    static int stringStep = 1;
    static int samplesUntilNextString = 0;
    static bool strumInProgress = false;

    constexpr int stringDelaySamples = 900; // ~20 ms at 44.1 kHz

    if (globalState->guitarStrumHit.exchange(false)) {
        
        int activeRoot = static_cast<int>(globalState->currentGuitarRoot.load());
        int activeQuality = static_cast<int>(globalState->currentGuitarQuality.load());

        // Bounds check (possible source of previous seg fault)
        if (activeRoot >= 0 && activeRoot < 12 && activeQuality >= 0 && activeQuality < 7) {
            
            const int* chord = airchestra::getGuitarChord(
                globalState->currentGuitarRoot.load(),
                globalState->currentGuitarQuality.load()
            );

            if (chord != nullptr) {
                for (int i = 0; i < 6; ++i) {
                    pendingNotes[i] = chord[i];
                }

                const auto strumDirection = globalState->guitarStrumDirection.load();
                pendingVelocity = globalState->guitarVelocity.load();
                nextStringIndex = strumDirection == GuitarStrumDirection::Up ? 5 : 0;
                stringStep = strumDirection == GuitarStrumDirection::Up ? -1 : 1;
                samplesUntilNextString = 0;
                strumInProgress = true;
            }
        }
    }

    if (strumInProgress) {
        int samplesProcessed = 0;

        while (samplesProcessed < numSamples && nextStringIndex >= 0 && nextStringIndex < 6) {
            if (samplesUntilNextString <= 0) {
                const int note = pendingNotes[nextStringIndex];

                if (note >= 0) {
                    guitarSynth.noteOn(0, note, pendingVelocity);

                    if (midiOut != nullptr) {
                        midiOut->sendMessageNow(
                            juce::MidiMessage::noteOn(1, note, (juce::uint8)pendingVelocity)
                        );
                    }
                }

                nextStringIndex += stringStep;
                samplesUntilNextString = stringDelaySamples;
            }

            const int advance = juce::jmin(samplesUntilNextString, numSamples - samplesProcessed);
            samplesUntilNextString -= advance;
            samplesProcessed += advance;
        }

        if (nextStringIndex < 0 || nextStringIndex >= 6)
            strumInProgress = false;
    }

    float* outChannels[] = {
        buffer.getWritePointer(0),
        buffer.getWritePointer(1)
    };

    guitarSynth.renderBlock(outChannels, numSamples);
}
