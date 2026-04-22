// Audio engine implementation.
// Manages audio device setup, real-time processing,
// instrument rendering, and MIDI output.

#include "AudioEngine.h"
#include <iostream>
#include <cmath>

namespace
{
    // Translates custom SM Drums notes back into standard General MIDI
    int translateSMtoGM(int smNote) 
    {
        switch(smNote) 
        {
            case 43: return 41; // Floor Tom: translates SM 43 to GM 41
            case 48: return 47; // High Tom: translates SM 50 to GM 48
            case 54: return 49; // Crash Cymbal: translates SM 54 to GM 49
            case 60: return 51; // Ride Cymbal: translates SM 60 to GM 51
            
            // Kick (36), Snare (38), and Hi-Hats (42, 46), Low Tom (45) are already GM standard
            default: return smNote; 
        }
    }
}

// Initializes the audio engine from precomputed startup configuration.
HeadlessAudioEngine::HeadlessAudioEngine(GlobalState* statePtr, const AudioEngineConfig& config)
    : globalState(statePtr)
{
    auto midiOutputs = juce::MidiOutput::getAvailableDevices();

    if (config.enableMidiOut && !midiOutputs.isEmpty())
    {
        if (config.midiDeviceIndex >= 0 && config.midiDeviceIndex < static_cast<int>(midiOutputs.size()))
        {
            midiOut = juce::MidiOutput::openDevice(midiOutputs[config.midiDeviceIndex].identifier);
        }
        else
        {
            midiOut = nullptr;
        }
    }
    else
    {
        midiOut = nullptr;
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

// Cleans up audio resources by removing the audio callback
HeadlessAudioEngine::~HeadlessAudioEngine()
{
    deviceManager.removeAudioCallback(this);
}

// Returns whether MIDI output is currently active.
bool HeadlessAudioEngine::isMidiEnabled() const
{
    return midiOut != nullptr;
}

// Called when audio device starts
// Logs device configuration, propagates sample rate and buffer size to all synth engines
void HeadlessAudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    std::cout << "Audio Device: " << device->getName() << std::endl;
    std::cout << "Sample Rate: " << device->getCurrentSampleRate() << std::endl;
    std::cout << "Buffer Size: " << device->getCurrentBufferSizeSamples() << std::endl;
    std::cout << "Output Latency (ms): "
              << device->getOutputLatencyInSamples() * 1000.0 / device->getCurrentSampleRate()
              << std::endl;

    synth.setCurrentPlaybackSampleRate(device->getCurrentSampleRate());

    {
        std::lock_guard<std::mutex> lock(drumSynthMutex);
        drumSynth.setSampleRate(device->getCurrentSampleRate());
        drumSynth.setSamplesPerBlock(device->getCurrentBufferSizeSamples());
    }

    {
        std::lock_guard<std::mutex> lock(keyboardSynthMutex);
        keyboardSynth.setSampleRate(device->getCurrentSampleRate());
        keyboardSynth.setSamplesPerBlock(device->getCurrentBufferSizeSamples());
    }
}

// Called when audio device stops (not implemented)
void HeadlessAudioEngine::audioDeviceStopped() {}

// Resets drum playback/trigger state so stale hits are not consumed
void HeadlessAudioEngine::resetDrumPlaybackState()
{
    if (globalState == nullptr)
        return;

    globalState->leftDrumHit.store(false);
    globalState->rightDrumHit.store(false);
    globalState->leftDrumType.store(36);
    globalState->rightDrumType.store(38);
    globalState->leftDrumVelocity.store(100);
    globalState->rightDrumVelocity.store(100);
}

// Resets keyboard playback state so stale notes/pedal are not reused
void HeadlessAudioEngine::resetKeyboardPlaybackState()
{
    if (globalState == nullptr)
        return;

    globalState->isKeyPressed.store(false);
    globalState->keyboardNote.store(60);
    globalState->keyboardVelocity.store(100);
    globalState->sustainPedal.store(false);

    wasKeyPressed = false;
    wasSustainPedalPressed = false;
    lastPlayedKey = -1;
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

    // Clear stale trigger state before changing kit
    resetDrumPlaybackState();

    std::cout << "Loading drum SFZ: " << sfzPath << std::endl;

    bool loaded = drumSynth.loadSfzFile(sfzPath.toStdString());
    if (!loaded) {
        std::cerr << "ERROR: Failed to load SFZ: " << sfzPath << std::endl;
        return;
    }

    loadedDrumSfzPath = sfzPath;
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

    // Clear stale keyboard state before changing patch
    resetKeyboardPlaybackState();

    std::cout << "Loading keyboard SFZ: " << sfzToLoad << std::endl;

    bool loaded = keyboardSynth.loadSfzFile(sfzToLoad.toStdString());
    if (!loaded) {
        std::cerr << "ERROR: Failed to load SFZ: " << sfzToLoad << std::endl;
        return;
    }

    loadedKeyboardInstrumentID = keyboardInstrumentID;
}

// Processes the theremin
void HeadlessAudioEngine::processTheremin(juce::AudioBuffer<float>& buffer, int numSamples)
{
    const bool isRightVisible = globalState->rightHandVisible.load();
    const bool isLeftVisible = globalState->leftHandVisible.load();

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
        const float x = globalState->rightHandX.load();
        const float y = globalState->leftHandY.load();
        const float semitoneRangeOneSide = static_cast<float>(globalState->thereminSemitoneRangeOneSide.load());
        const float centerMidiNote = static_cast<float>(globalState->thereminCenterNote.load());

        float normalizedX = x / 0.67f;
        if (normalizedX > 1.0f) normalizedX = 1.0f;
        if (normalizedX < 0.0f) normalizedX = 0.0f;

        const float semitonesFromCenter = (x * (semitoneRangeOneSide*2.0f)) - (semitoneRangeOneSide);
        const float targetMidiNote = centerMidiNote + semitonesFromCenter;
        const double targetFreq = 440.0f * std::pow(2.0f, (targetMidiNote - 69.0f) / 12.0f);;

        const float safeY = juce::jlimit(0.0f, 1.0f, y);
        const float targetVol = isLeftVisible ? (1.0f - safeY) : 0.0f;

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

// Processes the drums
void HeadlessAudioEngine::processDrums(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (buffer.getNumChannels() < 2)
        return;

    std::lock_guard<std::mutex> lock(drumSynthMutex);

    if (globalState->leftDrumHit.exchange(false)) {
        const int leftNote = globalState->leftDrumType.load();
        const int leftVelocity = globalState->leftDrumVelocity.load();
        int standardLeftGMNote = translateSMtoGM(leftNote);

        drumSynth.noteOn(0, leftNote, leftVelocity);
        if (midiOut != nullptr)
            midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, standardLeftGMNote, (juce::uint8)leftVelocity));
    }

    if (globalState->rightDrumHit.exchange(false)) {
        const int rightNote = globalState->rightDrumType.load();
        const int rightVelocity = globalState->rightDrumVelocity.load();
        int standardRightGMNote = translateSMtoGM(rightNote);

        drumSynth.noteOn(0, rightNote, rightVelocity);
        if (midiOut != nullptr)
            midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, standardRightGMNote, (juce::uint8)rightVelocity));
    }

    float* outChannels[] = { buffer.getWritePointer(0), buffer.getWritePointer(1) };
    drumSynth.renderBlock(outChannels, numSamples);
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

    //Handle notes
    const bool isPressed = globalState->isKeyPressed.load();
    const int currentNote = globalState->keyboardNote.load();
    const int velocity = globalState->keyboardVelocity.load();

    if (isPressed && !wasKeyPressed) {
        keyboardSynth.noteOn(0, currentNote, velocity);

        if (midiOut != nullptr) {
            midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, currentNote, (juce::uint8)velocity));
        }

        lastPlayedKey = currentNote;
    }
    else if (!isPressed && wasKeyPressed) {
        keyboardSynth.noteOff(0, lastPlayedKey, 0);

        if (midiOut != nullptr) {
            midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, lastPlayedKey, (juce::uint8)0));
        }

        lastPlayedKey = -1;
    }
    else if (isPressed && wasKeyPressed && currentNote != lastPlayedKey) {
        keyboardSynth.noteOff(0, lastPlayedKey, 0);
        if (midiOut != nullptr) {
            midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, lastPlayedKey, (juce::uint8)0));
        }

        keyboardSynth.noteOn(0, currentNote, velocity);
        if (midiOut != nullptr) {
            midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, currentNote, (juce::uint8)velocity));
        }

        lastPlayedKey = currentNote;
    }

    wasKeyPressed = isPressed;

    float* outChannels[] = { buffer.getWritePointer(0), buffer.getWritePointer(1) };
    keyboardSynth.renderBlock(outChannels, numSamples);
}

// Main real-time audio callback
// Clears output buffer, dispatches processing based on selected instrument
void HeadlessAudioEngine::audioDeviceIOCallbackWithContext(
    const float* const*, int,
    float* const* outputChannelData, int numOutputChannels,
    int numSamples, const juce::AudioIODeviceCallbackContext&)
{
    juce::AudioBuffer<float> buffer(const_cast<float**>(outputChannelData), numOutputChannels, numSamples);
    buffer.clear();

    switch (globalState->currentInstrument.load()) {
        case ActiveInstrument::Theremin:
            processTheremin(buffer, numSamples);
            break;

        case ActiveInstrument::Drums:
            processDrums(buffer, numSamples);
            break;

        case ActiveInstrument::Keyboard:
            processKeyboard(buffer, numSamples);
            break;
    }

    const float masterGain = juce::jlimit(0.0f, 1.0f, globalState->masterVolume.load());
    buffer.applyGain(masterGain);
}