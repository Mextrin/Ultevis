/*
==============================================================================
AUDIO ENGINE IMPLEMENTATION (Real-Time Processing & Signal Flow)

This file implements the top-level audio engine. It is responsible for device
setup, MIDI output setup, SFZ instrument loading, and real-time processing in
the audio callback.

It does not implement waveform generation itself; that logic lives in the
separate oscillator voice module.
==============================================================================
*/

#include "AudioEngine.h"
#include <iostream>
#include <cmath>

// =============================================================================
// HEADLESS AUDIO ENGINE IMPLEMENTATION
// =============================================================================
HeadlessAudioEngine::HeadlessAudioEngine(GlobalState* statePtr) : globalState(statePtr)
{
    bool midiOutSwitch = globalState->routeToMidiOut.load();
    auto midiOutputs = juce::MidiOutput::getAvailableDevices();

    if (midiOutSwitch && midiOutputs.size() > 0) {
        std::cout << "\nAvailable MIDI output devices:\n";
        for (size_t i = 0; i < midiOutputs.size(); ++i) {
            std::cout << "[" << i << "] " << midiOutputs[i].name.toStdString() << '\n';
        }

        std::cout << "\nSelect device number (-1 to disable): ";
        int choice = -1;
        std::cin >> choice;

        if (choice >= 0 && choice < static_cast<int>(midiOutputs.size())) {
            midiOut = juce::MidiOutput::openDevice(midiOutputs[choice].identifier);
            std::cout << "\nSUCCESS: Connected to MIDI Port -> "
                      << midiOutputs[choice].name.toStdString() << "\n\n";
        } else {
            midiOut = nullptr;
            std::cout << "\nMIDI Disabled" << std::endl;
        }
    }
    else if (midiOutSwitch && midiOutputs.size() == 0) {
        midiOut = nullptr;
        std::cout << "\nUNSUCCESSFUL: No Midi Port" << std::endl;
    }
    else {
        midiOut = nullptr;
        std::cout << "\nMIDI Switch Off" << std::endl;
    }

    synth.addVoice(new SineWaveVoice());
    synth.addSound(new SineWaveSound());

    deviceManager.initialise(0, 2, nullptr, true);

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);

    setup.bufferSize = 256;
    setup.sampleRate = 44100.0;
    setup.useDefaultInputChannels = false;
    setup.useDefaultOutputChannels = true;

    juce::String error = deviceManager.setAudioDeviceSetup(setup, true);
    if (error.isNotEmpty())
        std::cerr << "Audio device setup error: " << error.toStdString() << std::endl;

    deviceManager.addAudioCallback(this);
}

HeadlessAudioEngine::~HeadlessAudioEngine()
{
    deviceManager.removeAudioCallback(this);
}

void HeadlessAudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    std::cout << "Audio Device: " << device->getName() << std::endl;
    std::cout << "Sample Rate: " << device->getCurrentSampleRate() << std::endl;
    std::cout << "Buffer Size: " << device->getCurrentBufferSizeSamples() << std::endl;
    std::cout << "Output Latency (ms): "
              << device->getOutputLatencyInSamples() * 1000.0 / device->getCurrentSampleRate()
              << std::endl;

    synth.setCurrentPlaybackSampleRate(device->getCurrentSampleRate());

    drumSynth.setSampleRate(device->getCurrentSampleRate());
    drumSynth.setSamplesPerBlock(device->getCurrentBufferSizeSamples());

    keyboardSynth.setSampleRate(device->getCurrentSampleRate());
    keyboardSynth.setSamplesPerBlock(device->getCurrentBufferSizeSamples());
}

void HeadlessAudioEngine::audioDeviceStopped() {}

void HeadlessAudioEngine::loadDrumSound(const juce::String& sfzPath)
{
    bool loaded = drumSynth.loadSfzFile(sfzPath.toStdString());
    if (!loaded) {
        std::cerr << "ERROR: Failed to load SFZ: " << sfzPath << std::endl;
    }
}

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
        sfzToLoad = baseOrchestraPath + "Glockenspiel.sfz";
    }
    else if (keyboardInstrumentID == 3) { 
        sfzToLoad = baseOrchestraPath + "Harp.sfz";
    }
    else if (keyboardInstrumentID == 4) { 
        sfzToLoad = baseOrchestraPath + "ViolinEnsSusVib.sfz";
    }

    if (sfzToLoad.isNotEmpty()) {
        bool loaded = keyboardSynth.loadSfzFile(sfzToLoad.toStdString());
        if (!loaded) {
            std::cerr << "ERROR: Failed to load SFZ: " << sfzToLoad << std::endl;
        }
    }
}

void HeadlessAudioEngine::audioDeviceIOCallbackWithContext(
    const float* const*, int,
    float* const* outputChannelData, int numOutputChannels,
    int numSamples, const juce::AudioIODeviceCallbackContext&)
{
    juce::AudioBuffer<float> buffer(const_cast<float**>(outputChannelData), numOutputChannels, numSamples);
    buffer.clear();

    auto activeInst = globalState->currentInstrument.load();

    if (activeInst == ActiveInstrument::Theremin) {
        bool isRightVisible = globalState->rightHandVisible.load();
        bool isLeftVisible = globalState->leftHandVisible.load();

        if (isRightVisible && !wasRightVisible) {
            synth.noteOn(1, 60, 1.0f);
            if (midiOut != nullptr) {
                midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, 60, 1.0f));
            }
        }
        else if (!isRightVisible && wasRightVisible) {
            synth.noteOff(1, 60, 1.0f, true);
            if (midiOut != nullptr) {
                midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, 60, 0.0f));
            }
        }
        wasRightVisible = isRightVisible;

        if (isRightVisible) {
            float x = globalState->rightHandX.load();
            float y = globalState->leftHandY.load();

            float semitonesFromCenter = (x * 48.0f) - 24.0f;
            double targetFreq = 261.625565 * std::pow(2.0, semitonesFromCenter / 12.0);

            float safeY = juce::jlimit(0.0f, 1.0f, y);
            float targetVol = isLeftVisible ? (1.0f - safeY) : 0.0f;

            if (auto* myVoice = dynamic_cast<SineWaveVoice*>(synth.getVoice(0))) {
                myVoice->setWaveform(globalState->currentWaveform.load());
                myVoice->updateThereminMath(targetFreq, targetVol);
            }

            if (midiOut != nullptr) {
                int midiPitchBend = static_cast<int>(x * 16383.0f);
                int midiVolume = static_cast<int>(targetVol * 127.0f);

                midiOut->sendMessageNow(juce::MidiMessage::pitchWheel(1, midiPitchBend));
                midiOut->sendMessageNow(juce::MidiMessage::controllerEvent(1, 7, midiVolume));
            }
        }

        synth.renderNextBlock(buffer, juce::MidiBuffer(), 0, numSamples);
    }
    else if (activeInst == ActiveInstrument::Drums) {
        if (globalState->leftDrumHit.exchange(false)) {
            int leftNote = globalState->leftDrumType.load();
            int leftVelocity = globalState->leftDrumVelocity.load();

            drumSynth.noteOn(0, leftNote, leftVelocity);
            if (midiOut != nullptr)
                midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, leftNote, (juce::uint8)leftVelocity));
        }

        if (globalState->rightDrumHit.exchange(false)) {
            int rightNote = globalState->rightDrumType.load();
            int rightVelocity = globalState->rightDrumVelocity.load();

            drumSynth.noteOn(0, rightNote, rightVelocity);
            if (midiOut != nullptr)
                midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, rightNote, (juce::uint8)rightVelocity));
        }

        float* outChannels[] = { buffer.getWritePointer(0), buffer.getWritePointer(1) };
        drumSynth.renderBlock(outChannels, numSamples);
    }
    else if (activeInst == ActiveInstrument::Keyboard) {
        bool isPressed = globalState->isKeyPressed.load();
        int currentNote = globalState->keyboardNote.load();
        int velocity = globalState->keyboardVelocity.load();

        // Strike, hand down
        if (isPressed && !wasKeyPressed) {
            keyboardSynth.noteOn(0, currentNote, velocity);
            
            if (midiOut != nullptr) {
                midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, currentNote, (juce::uint8)velocity));
            }
            lastPlayedKey = currentNote; // Remember what we just played
        }
        // Release, hand just lifted up 
        else if (!isPressed && wasKeyPressed) {
            keyboardSynth.noteOff(0, lastPlayedKey, 0); // Turn off the exact key we remembered
            
            if (midiOut != nullptr) {
                midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, lastPlayedKey, (juce::uint8)0));
            }
            lastPlayedKey = -1; // Clear the memory
        }
        // Glissando
        else if (isPressed && wasKeyPressed && currentNote != lastPlayedKey) {
            
            // Turn off old key immediately 
            keyboardSynth.noteOff(0, lastPlayedKey, 0);
            if (midiOut != nullptr) {
                midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, lastPlayedKey, (juce::uint8)0));
            }

            // Turn on new key
            keyboardSynth.noteOn(0, currentNote, velocity);
            if (midiOut != nullptr) {
                midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, currentNote, (juce::uint8)velocity));
            }
            
            // Update memory to new key
            lastPlayedKey = currentNote;
        }

        wasKeyPressed = isPressed;

        float* outChannels[] = { buffer.getWritePointer(0), buffer.getWritePointer(1) };
        keyboardSynth.renderBlock(outChannels, numSamples);
    }
}