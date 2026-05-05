#include "AudioEngine.h"

#include <algorithm>
#include <iostream>

namespace
{
    int midiVelocityFromKeyboardStore(int stored)
    {
        return std::clamp(stored, 1, 127);
    }
}

// Resets keyboard playback state so stale notes/pedal are not reused
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

// Loads selected keyboard instrument based on ID
void HeadlessAudioEngine::loadKeyboardSound(int keyboardInstrumentID)
{
    juce::String baseOrchestraPath = "Instruments/VSCO-2-CE/";
    juce::String sfzToLoad = "";

    if (keyboardInstrumentID == 0) {
        // Grand Piano
        sfzToLoad = "Instruments/AccurateSalamanderGrandPianoV6.2beta2_48khz24bit/sfz_live/Accurate-SalamanderGrandPiano_flat.Recommended.sfz";
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

    //inject sustain values into sfz files for violin, flute, organ
    if (keyboardInstrumentID == 1 || keyboardInstrumentID == 2 || keyboardInstrumentID == 4) {
        juce::String patch = "\nampeg_decay=4.0\nampeg_sustain=0\n";
        content = content.replace("<global>", "<global>" + patch)
                         .replace("<group>", "<group>" + patch);
    }

    // Load from memory using the absolute path so sfizz can find the .wav folders
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

    // Sustain Pedal
    const bool isPedalPressed = globalState->sustainPedal.load();

    if (isPedalPressed != wasSustainPedalPressed) {
        int midiPedalValue = isPedalPressed ? 127 : 0;

        // Send CC 64 to the internal sfizz engine (0 is the delay argument)
        keyboardSynth.cc(0, 64, midiPedalValue);

        // Send CC 64 to Ableton/DAW
        if (midiOut != nullptr) {
            midiOut->sendMessageNow(juce::MidiMessage::controllerEvent(1, 64, midiPedalValue));
        }

        wasSustainPedalPressed = isPedalPressed;
    }

    for (int i = 0; i < 128; ++i) {
        bool isPressed = globalState->keyboardState[i].load();
        
        // If GlobalState different from internal memory, it is a new press/release
        if (isPressed != internalKeyboardState[i]) {
            internalKeyboardState[i] = isPressed; // Sync memory
            
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