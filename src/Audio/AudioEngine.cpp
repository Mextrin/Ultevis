#include "AudioEngine.h"
#include <iostream>
#include <cmath>

HeadlessAudioEngine::HeadlessAudioEngine(GlobalState* statePtr) : globalState(statePtr)
{
    synth.addVoice(new SineWaveVoice());
    synth.addSound(new SineWaveSound());

    deviceManager.initialise(0, 2, nullptr, true);

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);
    setup.bufferSize              = 256;
    setup.sampleRate              = 44100.0;
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

void HeadlessAudioEngine::loadDrumSound(const juce::String& sfzPath)
{
    if (!drumSynth.loadSfzFile(sfzPath.toStdString()))
        std::cerr << "Failed to load drum SFZ: " << sfzPath << "\n";
}

void HeadlessAudioEngine::loadKeyboardSound(int id)
{
    juce::String path;
    switch (id) {
        case 0: path = "Instruments/AccurateSalamanderGrandPianoV6.2beta2_48khz24bit/sfz_live/Accurate-SalamanderGrandPiano_flat.Recommended.sfz"; break;
        case 1: path = "Instruments/VSCO-2-CE/OrganLoud.sfz";       break;
        case 2: path = "Instruments/VSCO-2-CE/FluteSusVib.sfz";     break;
        case 3: path = "Instruments/VSCO-2-CE/Harp.sfz";            break;
        case 4: path = "Instruments/VSCO-2-CE/ViolinEnsSusVib.sfz"; break;
        default: return;
    }
    if (!keyboardSynth.loadSfzFile(path.toStdString()))
        std::cerr << "Failed to load keyboard SFZ: " << path << "\n";
}

void HeadlessAudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const double sr = device->getCurrentSampleRate();
    const int    bs = device->getCurrentBufferSizeSamples();
    synth.setCurrentPlaybackSampleRate(sr);
    drumSynth.setSampleRate(sr);
    drumSynth.setSamplesPerBlock(bs);
    keyboardSynth.setSampleRate(sr);
    keyboardSynth.setSamplesPerBlock(bs);
}

void HeadlessAudioEngine::audioDeviceStopped() {}

// ---------------------------------------------------------------------------
// Theremin — logarithmic pitch mapping driven by rightHandX,
//            volume-floor-aware gain driven by leftHandY.
// ---------------------------------------------------------------------------
void HeadlessAudioEngine::processTheremin(juce::AudioBuffer<float>& buffer, int numSamples)
{
    const bool isRightVisible = globalState->rightHandVisible.load();
    const bool isLeftVisible  = globalState->leftHandVisible.load();

    if (isRightVisible && !wasRightVisible) {
        synth.noteOn(1, 60, 1.0f);
        if (midiOut) midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, 60, 1.0f));
    } else if (!isRightVisible && wasRightVisible) {
        synth.noteOff(1, 60, 1.0f, true);
        if (midiOut) midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, 60, 0.0f));
    }
    wasRightVisible = isRightVisible;

    if (isRightVisible) {
        const float x    = globalState->rightHandX.load();
        const float y    = globalState->leftHandY.load();
        const float fMin = globalState->freqMin.load();
        const float fMax = globalState->freqMax.load();

        // Log-scale mapping: x=0 → fMin, x=1 → fMax
        const double targetFreq = (fMin > 0.0f && fMax > fMin)
            ? fMin * std::pow(static_cast<double>(fMax) / fMin, static_cast<double>(x))
            : 261.625565 * std::pow(2.0, (x * 48.0 - 24.0) / 12.0);

        const float safeY    = juce::jlimit(0.0f, 1.0f, y);
        const float rawVol   = isLeftVisible ? (1.0f - safeY) : 0.0f;
        const float vFloor   = globalState->volumeFloor.load();
        // Scale raw 0–1 into [vFloor, 1] so there's always some sound when hand is visible
        const float targetVol = rawVol > 0.001f ? vFloor + rawVol * (1.0f - vFloor) : 0.0f;

        if (auto* voice = dynamic_cast<SineWaveVoice*>(synth.getVoice(0))) {
            voice->setWaveform(globalState->currentWaveform.load());
            voice->updateThereminMath(targetFreq, targetVol);
        }

        if (midiOut) {
            midiOut->sendMessageNow(juce::MidiMessage::pitchWheel(1, static_cast<int>(x * 16383.0f)));
            midiOut->sendMessageNow(juce::MidiMessage::controllerEvent(1, 7, static_cast<int>(targetVol * 127.0f)));
        }
    }

    synth.renderNextBlock(buffer, juce::MidiBuffer(), 0, numSamples);
}

void HeadlessAudioEngine::processDrums(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (globalState->leftDrumHit.exchange(false)) {
        const int note = globalState->leftDrumType.load();
        const int vel  = globalState->leftDrumVelocity.load();
        drumSynth.noteOn(0, note, vel);
        if (midiOut) midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, note, (juce::uint8)vel));
    }
    if (globalState->rightDrumHit.exchange(false)) {
        const int note = globalState->rightDrumType.load();
        const int vel  = globalState->rightDrumVelocity.load();
        drumSynth.noteOn(0, note, vel);
        if (midiOut) midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, note, (juce::uint8)vel));
    }
    float* ch[] = { buffer.getWritePointer(0), buffer.getWritePointer(1) };
    drumSynth.renderBlock(ch, numSamples);
}

void HeadlessAudioEngine::processKeyboard(juce::AudioBuffer<float>& buffer, int numSamples)
{
    const bool pedal = globalState->sustainPedal.load();
    if (pedal != wasSustainPedalPressed) {
        int val = pedal ? 127 : 0;
        keyboardSynth.cc(0, 64, val);
        if (midiOut) midiOut->sendMessageNow(juce::MidiMessage::controllerEvent(1, 64, val));
        wasSustainPedalPressed = pedal;
    }

    const bool isPressed   = globalState->isKeyPressed.load();
    const int  currentNote = globalState->keyboardNote.load();
    const int  velocity    = globalState->keyboardVelocity.load();

    if (isPressed && !wasKeyPressed) {
        keyboardSynth.noteOn(0, currentNote, velocity);
        if (midiOut) midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, currentNote, (juce::uint8)velocity));
        lastPlayedKey = currentNote;
    } else if (!isPressed && wasKeyPressed) {
        keyboardSynth.noteOff(0, lastPlayedKey, 0);
        if (midiOut) midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, lastPlayedKey, (juce::uint8)0));
        lastPlayedKey = -1;
    } else if (isPressed && wasKeyPressed && currentNote != lastPlayedKey) {
        keyboardSynth.noteOff(0, lastPlayedKey, 0);
        if (midiOut) midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, lastPlayedKey, (juce::uint8)0));
        keyboardSynth.noteOn(0, currentNote, velocity);
        if (midiOut) midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, currentNote, (juce::uint8)velocity));
        lastPlayedKey = currentNote;
    }
    wasKeyPressed = isPressed;

    float* ch[] = { buffer.getWritePointer(0), buffer.getWritePointer(1) };
    keyboardSynth.renderBlock(ch, numSamples);
}

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
