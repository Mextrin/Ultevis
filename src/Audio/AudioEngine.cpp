#include "AudioEngine.h"
#include <iostream>

#if JUCE_MAC
static OSStatus onDefaultDeviceChanged(AudioObjectID, UInt32,
                                       const AudioObjectPropertyAddress*,
                                       void* clientData)
{
    auto* engine = static_cast<HeadlessAudioEngine*>(clientData);
    juce::MessageManager::callAsync([engine] { engine->startTimer(500); });
    return noErr;
}
#endif

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

    deviceManager.setAudioDeviceSetup(setup, true);

    deviceManager.addAudioCallback(this);
    deviceManager.addChangeListener(this);

#if JUCE_MAC
    {
        AudioObjectPropertyAddress addr {
            kAudioHardwarePropertyDefaultOutputDevice,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMain
        };
        AudioObjectAddPropertyListener(kAudioObjectSystemObject, &addr,
                                       onDefaultDeviceChanged, this);
    }
#endif
}

HeadlessAudioEngine::~HeadlessAudioEngine()
{
#if JUCE_MAC
    {
        AudioObjectPropertyAddress addr {
            kAudioHardwarePropertyDefaultOutputDevice,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMain
        };
        AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &addr,
                                          onDefaultDeviceChanged, this);
    }
#endif

    deviceManager.removeChangeListener(this);
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

    {
        std::lock_guard<std::mutex> lock(drumSynthMutex);
        drumSynth.setSampleRate(device->getCurrentSampleRate());
        drumSynth.setSamplesPerBlock(device->getCurrentBufferSizeSamples());
        drumSynth.setNumVoices(12); //limit drum voices
    }

    {
        std::lock_guard<std::mutex> lock(keyboardSynthMutex);
        keyboardSynth.setSampleRate(device->getCurrentSampleRate());
        keyboardSynth.setSamplesPerBlock(device->getCurrentBufferSizeSamples());
        keyboardSynth.setNumVoices(16); //limit piano voices
    }
}

void HeadlessAudioEngine::audioDeviceStopped()
{
    if (!isReinitializing)
        startTimer(500);
}

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