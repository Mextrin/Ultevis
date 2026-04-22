#include "AudioEngine.h"
#include <iostream>
#include <cmath>

HeadlessAudioEngine::HeadlessAudioEngine(GlobalState* statePtr) : globalState(statePtr)
{
    // Theremin — one continuously-updated voice
    thereminSynth.addVoice(new SineWaveVoice());
    thereminSynth.addSound(new SineWaveSound());

    // Keyboard — 8 polyphonic voices, MIDI-pitch-driven
    for (int i = 0; i < 8; ++i)
        keySynth.addVoice(new KeyboardVoice());
    keySynth.addSound(new KeySynthSound());

    // Drums — 8 simultaneous percussive voices
    for (int i = 0; i < 8; ++i)
        drumSynth.addVoice(new DrumVoice());
    drumSynth.addSound(new DrumSynthSound());

    // Pre-seed sample rate so any noteOn before audioDeviceAboutToStart
    // doesn't compute cyclesPerSample = freq / 0.
    constexpr double kDefaultSR = 44100.0;
    thereminSynth.setCurrentPlaybackSampleRate(kDefaultSR);
    keySynth.setCurrentPlaybackSampleRate(kDefaultSR);
    drumSynth.setCurrentPlaybackSampleRate(kDefaultSR);

    deviceManager.initialise(0, 2, nullptr, true);

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);
    setup.bufferSize               = 256;
    setup.sampleRate               = kDefaultSR;
    setup.useDefaultInputChannels  = false;
    setup.useDefaultOutputChannels = true;

    juce::String error = deviceManager.setAudioDeviceSetup(setup, true);
    if (error.isNotEmpty())
        std::cerr << "Audio device setup error: " << error.toStdString() << "\n";

    deviceManager.addAudioCallback(this);
}

HeadlessAudioEngine::~HeadlessAudioEngine()
{
    deviceManager.removeAudioCallback(this);
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
        midiOut = nullptr;
        return;
    }
    midiOut = juce::MidiOutput::openDevice(juce::String(identifier));
    if (!midiOut)
        std::cerr << "MIDI: failed to open device " << identifier << "\n";
}

void HeadlessAudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const double sr = device->getCurrentSampleRate();
    thereminSynth.setCurrentPlaybackSampleRate(sr);
    keySynth.setCurrentPlaybackSampleRate(sr);
    drumSynth.setCurrentPlaybackSampleRate(sr);
}

void HeadlessAudioEngine::audioDeviceStopped() {}

// ---------------------------------------------------------------------------
// Theremin
// ---------------------------------------------------------------------------
void HeadlessAudioEngine::processTheremin(juce::AudioBuffer<float>& buffer, int numSamples)
{
    const bool isRightVisible = globalState->rightHandVisible.load();
    const bool isLeftVisible  = globalState->leftHandVisible.load();

    if (isRightVisible) {
        thereminInvisibleCount = 0;
        if (!wasRightVisible) {
            thereminSynth.noteOn(1, 60, 1.0f);
            if (midiOut) midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, 60, 1.0f));
            wasRightVisible = true;
        }
    } else if (wasRightVisible) {
        // Debounce: only call noteOff after the hand has been absent for
        // kInvisibleDebounce consecutive callbacks (~47 ms).  Single missed
        // MediaPipe frames are silently absorbed.
        if (++thereminInvisibleCount >= kInvisibleDebounce) {
            thereminSynth.noteOff(1, 60, 1.0f, true);
            if (midiOut) midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, 60, 0.0f));
            wasRightVisible       = false;
            thereminInvisibleCount = 0;
        }
    }

    if (wasRightVisible) {
        const float x    = globalState->rightHandX.load();
        const float y    = globalState->leftHandY.load();
        const float fMin = globalState->freqMin.load();
        const float fMax = globalState->freqMax.load();

        const double targetFreq = (fMin > 0.0f && fMax > fMin)
            ? fMin * std::pow(static_cast<double>(fMax) / fMin, static_cast<double>(x))
            : 261.625565 * std::pow(2.0, (x * 48.0 - 24.0) / 12.0);

        const float safeY     = juce::jlimit(0.0f, 1.0f, y);
        const float rawVol    = isLeftVisible ? (1.0f - safeY) : 0.0f;
        const float vFloor    = globalState->volumeFloor.load();
        const float targetVol = rawVol > 0.001f ? vFloor + rawVol * (1.0f - vFloor) : 0.0f;

        if (auto* voice = dynamic_cast<SineWaveVoice*>(thereminSynth.getVoice(0))) {
            voice->setWaveform(globalState->currentWaveform.load());
            voice->updateThereminMath(targetFreq, targetVol);
        }

        if (midiOut) {
            midiOut->sendMessageNow(juce::MidiMessage::pitchWheel(1, static_cast<int>(x * 16383.0f)));
            midiOut->sendMessageNow(juce::MidiMessage::controllerEvent(1, 7, static_cast<int>(targetVol * 127.0f)));
        }
    }

    thereminSynth.renderNextBlock(buffer, juce::MidiBuffer(), 0, numSamples);
}

// ---------------------------------------------------------------------------
// Drums
// ---------------------------------------------------------------------------
void HeadlessAudioEngine::processDrums(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (globalState->leftDrumHit.exchange(false)) {
        const int   note = globalState->leftDrumType.load();
        const float vel  = globalState->leftDrumVelocity.load() / 127.0f;
        drumSynth.noteOn(1, note, vel);
        if (midiOut) midiOut->sendMessageNow(
            juce::MidiMessage::noteOn(1, note, (juce::uint8)globalState->leftDrumVelocity.load()));
    }
    if (globalState->rightDrumHit.exchange(false)) {
        const int   note = globalState->rightDrumType.load();
        const float vel  = globalState->rightDrumVelocity.load() / 127.0f;
        drumSynth.noteOn(1, note, vel);
        if (midiOut) midiOut->sendMessageNow(
            juce::MidiMessage::noteOn(1, note, (juce::uint8)globalState->rightDrumVelocity.load()));
    }
    drumSynth.renderNextBlock(buffer, juce::MidiBuffer(), 0, numSamples);
}

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------
void HeadlessAudioEngine::processKeyboard(juce::AudioBuffer<float>& buffer, int numSamples)
{
    const bool pedal = globalState->sustainPedal.load();
    if (pedal != wasSustainPedalPressed) {
        if (midiOut)
            midiOut->sendMessageNow(juce::MidiMessage::controllerEvent(1, 64, pedal ? 127 : 0));
        wasSustainPedalPressed = pedal;
    }

    const bool isPressed   = globalState->isKeyPressed.load();
    const int  currentNote = globalState->keyboardNote.load();
    const float velocity   = globalState->keyboardVelocity.load() / 127.0f;

    if (isPressed && !wasKeyPressed) {
        keySynth.noteOn(1, currentNote, velocity);
        if (midiOut) midiOut->sendMessageNow(
            juce::MidiMessage::noteOn(1, currentNote, (juce::uint8)globalState->keyboardVelocity.load()));
        lastPlayedKey = currentNote;
    } else if (!isPressed && wasKeyPressed) {
        keySynth.noteOff(1, lastPlayedKey, 0.0f, true);
        if (midiOut) midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, lastPlayedKey, (juce::uint8)0));
        lastPlayedKey = -1;
    } else if (isPressed && wasKeyPressed && currentNote != lastPlayedKey) {
        keySynth.noteOff(1, lastPlayedKey, 0.0f, true);
        keySynth.noteOn(1, currentNote, velocity);
        if (midiOut) {
            midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, lastPlayedKey, (juce::uint8)0));
            midiOut->sendMessageNow(
                juce::MidiMessage::noteOn(1, currentNote, (juce::uint8)globalState->keyboardVelocity.load()));
        }
        lastPlayedKey = currentNote;
    }
    wasKeyPressed = isPressed;

    keySynth.renderNextBlock(buffer, juce::MidiBuffer(), 0, numSamples);
}

// ---------------------------------------------------------------------------
// Audio I/O callback
// ---------------------------------------------------------------------------
void HeadlessAudioEngine::audioDeviceIOCallbackWithContext(
    const float* const*, int,
    float* const* outputChannelData, int numOutputChannels,
    int numSamples, const juce::AudioIODeviceCallbackContext&)
{
    juce::AudioBuffer<float> buffer(const_cast<float**>(outputChannelData), numOutputChannels, numSamples);
    buffer.clear();

    switch (globalState->currentInstrument.load()) {
        case ActiveInstrument::Theremin: processTheremin(buffer, numSamples); break;
        case ActiveInstrument::Drums:    processDrums(buffer, numSamples);    break;
        case ActiveInstrument::Keyboard: processKeyboard(buffer, numSamples); break;
    }

    buffer.applyGain(juce::jlimit(0.0f, 1.0f, globalState->masterVolume.load()));
}
