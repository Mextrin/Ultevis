#include "AudioEngine.h"
#include "../airchestra/RuntimePaths.h"

#include <algorithm>
#include <iostream>

namespace
{
    int midiVelocityFromKeyboardStore(int stored)
    {
        return std::clamp(stored, 1, 127);
    }
}

// Resets keyboard playback state
void HeadlessAudioEngine::resetKeyboardPlaybackState()
{
    if (globalState == nullptr)
        return;

    globalState->sustainPedal.store(false);
    wasSustainPedalPressed = false;

    // Turn off all 128 notes
    for (int i = 0; i < 128; ++i) {
        globalState->keyboardState[i].store(false);
        globalState->keyboardNoteVelocity[i].store(100);
        if (internalKeyboardState[i]) {
            keyboardSynth.noteOff(0, i, 0);
            if (midiOut != nullptr) {
                midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, i, (juce::uint8)0));
            }
            internalKeyboardState[i] = false;
        }
    }
}

// Loads selected keyboard instrument
void HeadlessAudioEngine::loadKeyboardSound(int keyboardInstrumentID)
{
    const juce::String instrumentsPath = juce::String(airchestra::runtimePath("Instruments").toStdString()) + "/";
    juce::String baseOrchestraPath = instrumentsPath + "VSCO-2-CE/";
    juce::String sfzToLoad = "";

    if (keyboardInstrumentID == 0) {
        sfzToLoad = instrumentsPath + "AccurateSalamanderGrandPianoV6.2beta2_48khz24bit/sfz_live/Accurate-SalamanderGrandPiano_flat.Recommended.sfz";
    }
    else if (keyboardInstrumentID == 1) {
        sfzToLoad = baseOrchestraPath + "OrganLoud.sfz";
    }
    else if (keyboardInstrumentID == 2) {
        sfzToLoad = baseOrchestraPath + "FluteSusVib.sfz";
    }
    else if (keyboardInstrumentID == 3) {
        sfzToLoad = baseOrchestraPath + "Harp.sfz";
    }
    else if (keyboardInstrumentID == 4) {
        sfzToLoad = baseOrchestraPath + "ViolinEnsSusVib.sfz";
    }
    else {
        std::cerr << "ERROR: Invalid keyboard instrument ID: "
                  << keyboardInstrumentID << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(keyboardSynthMutex);

    // Avoid reloading the same keyboard patch repeatedly
    if (loadedKeyboardInstrumentID == keyboardInstrumentID) {
        std::cout << "Keyboard SFZ already loaded, skipping reload: "
                  << sfzToLoad << std::endl;
        resetKeyboardPlaybackState();
        return;
    }

    resetKeyboardPlaybackState();

    std::cout << "Loading keyboard SFZ: " << sfzToLoad << std::endl;

    juce::File sfzFile(sfzToLoad);
    juce::String content = sfzFile.loadFileAsString();

    if (keyboardInstrumentID == 1 || keyboardInstrumentID == 2 || keyboardInstrumentID == 4) {
        juce::String patch = "\nampeg_decay=4.0\nampeg_sustain=0\n";
        content = content.replace("<global>", "<global>" + patch)
                         .replace("<group>", "<group>" + patch);
    }

    bool loaded = keyboardSynth.loadSfzString(sfzFile.getFullPathName().toStdString(), content.toStdString());

    if (!loaded) {
        std::cerr << "ERROR: Failed to load SFZ: " << sfzToLoad << std::endl;
        return;
    }

    loadedKeyboardInstrumentID = keyboardInstrumentID;
}

// Processes the keyboard
void HeadlessAudioEngine::processKeyboard(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (buffer.getNumChannels() < 2)
        return;

    std::lock_guard<std::mutex> lock(keyboardSynthMutex);
    const bool isPedalPressed = globalState->sustainPedal.load();

    if (isPedalPressed != wasSustainPedalPressed) {
        int midiPedalValue = isPedalPressed ? 127 : 0;
        keyboardSynth.cc(0, 64, midiPedalValue);

        if (midiOut != nullptr) {
            midiOut->sendMessageNow(juce::MidiMessage::controllerEvent(1, 64, midiPedalValue));
        }

        wasSustainPedalPressed = isPedalPressed;
    }

    for (int i = 0; i < 128; ++i) {
        bool isPressed = globalState->keyboardState[i].load();
        
        if (isPressed != internalKeyboardState[i]) {
            internalKeyboardState[i] = isPressed;
            
            if (isPressed) {
                const int vel = midiVelocityFromKeyboardStore(globalState->keyboardNoteVelocity[i].load());
                keyboardSynth.noteOn(0, i, vel);
                if (midiOut != nullptr) {
                    midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, i, (juce::uint8)vel));
                }
            } else {
                keyboardSynth.noteOff(0, i, 0);
                if (midiOut != nullptr) {
                    midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, i, (juce::uint8)0));
                }
            }
        }
    }

    float* outChannels[] = { buffer.getWritePointer(0), buffer.getWritePointer(1) };
    keyboardSynth.renderBlock(outChannels, numSamples);
}
