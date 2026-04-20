/*
==============================================================================
AUDIO ENGINE IMPLEMENTATION (Real-Time Processing & Signal Flow)

This file implements the behavior defined in AudioEngine.h. It contains all
runtime logic required to transform hand-tracking input into audio output and
MIDI messages.

SineWaveVoice Implementation:
Defines how sound is actually generated. It initializes ADSR parameters,
handles note start/stop events, and continuously updates pitch and volume using
smoothed values to avoid audio artifacts. The renderNextBlock function runs on
the audio thread and produces a sine wave sample-by-sample, applying envelope
and volume scaling before writing to the output buffer.

HeadlessAudioEngine Implementation:
Controls the overall audio pipeline. It initializes the audio device and JUCE
synthesiser, then processes audio in the real-time callback. During each audio
cycle it:
- Reads hand position and visibility from GlobalState (lock-free)
- Triggers note-on and note-off events based on visibility changes
- Converts hand coordinates into frequency and amplitude values
- Pushes these values into the active voice
- Optionally converts gesture data into MIDI messages
- Renders the final audio buffer via the synthesiser

This file executes on the real-time audio thread and must remain deterministic
and efficient. It performs no blocking operations and avoids dynamic allocation
during audio processing.

In summary, this file contains the working logic that connects gesture input
to continuous sound generation and output.
==============================================================================
*/

#include "AudioEngine.h"
#include <iostream>
#include <cmath>

// ==============================================================================
// SINE WAVE VOICE IMPLEMENTATION
// ==============================================================================
SineWaveVoice::SineWaveVoice() 
{
    adsrParams.attack  = 0.1f;
    adsrParams.decay   = 0.1f;
    adsrParams.sustain = 1.0f;
    adsrParams.release = 0.5f; 
}

bool SineWaveVoice::canPlaySound (juce::SynthesiserSound* sound) {
    return dynamic_cast<SineWaveSound*> (sound) != nullptr;
}

void SineWaveVoice::startNote (int /*midiNoteNumber*/, float /*velocity*/, juce::SynthesiserSound*, int) 
{
    // Setup the smoothers to interpolate over 20 milliseconds (glitch prevention)
    smoothedFrequency.reset(getSampleRate(), 0.02);
    smoothedVolume.reset(getSampleRate(), 0.02);
    
    adsr.setParameters(adsrParams);
    adsr.noteOn(); 
}

void SineWaveVoice::stopNote (float, bool allowTailOff) 
{
    if (allowTailOff) adsr.noteOff(); 
    else clearCurrentNote();
}

void SineWaveVoice::updateThereminMath(double targetFreq, float targetVol) 
{
    // Update the smoothers with the new targets from the webcam
    smoothedFrequency.setTargetValue(targetFreq);
    smoothedVolume.setTargetValue(targetVol);
}

void SineWaveVoice::setWaveform(Waveform newWaveform)
{
    currentWaveform = newWaveform;
}

void SineWaveVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) 
{
    for (int i = 0; i < numSamples; ++i) {
        // 1. Advance the smoothers
        double currentFreq = smoothedFrequency.getNextValue();
        float currentVol = smoothedVolume.getNextValue();
        float currentAdsr = adsr.getNextSample();

        // 2. Calculate the oscillator phase
        auto cyclesPerSample = currentFreq / getSampleRate();
        auto angleDelta = cyclesPerSample * juce::MathConstants<double>::twoPi;
        
        currentAngle += angleDelta;
        if (currentAngle > juce::MathConstants<double>::twoPi)
            currentAngle -= juce::MathConstants<double>::twoPi;

        // 3. Generate the selected waveform sound
        // Multiply by 0.2f as a master safety volume so you don't blow the speakers
        float waveSample = 0.0f;
        double phase = currentAngle / juce::MathConstants<double>::twoPi;

        switch (currentWaveform) {
            case Waveform::Sine:
                waveSample = (float) std::sin(currentAngle);
                break;
            case Waveform::Square:
                waveSample = (std::sin(currentAngle) >= 0.0) ? 1.0f : -1.0f;
                break;
            case Waveform::Saw:
                waveSample = (float)(2.0 * phase - 1.0);
                break;
            case Waveform::Triangle:
                waveSample = (float)(2.0 * std::abs(2.0 * phase - 1.0) - 1.0);
                break;
        }

        float currentSample = waveSample * currentAdsr * currentVol * 0.2f;

        // 4. Write to speakers
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) {
            outputBuffer.addSample (channel, startSample + i, currentSample);
        }
    }

    if (!adsr.isActive()) clearCurrentNote();
}

// ==============================================================================
// HEADLESS AUDIO ENGINE IMPLEMENTATION
// ==============================================================================
HeadlessAudioEngine::HeadlessAudioEngine(GlobalState* statePtr) : globalState(statePtr)
{
    bool midiOutSwitch = globalState->routeToMidiOut.load(); // load from globalstate
    auto midiOutputs = juce::MidiOutput::getAvailableDevices(); // gets all midi devices
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
            std::cout << "\nSUCCESS: Connected to MIDI Port -> " << midiOutputs[choice].name.toStdString() << "\n\n";
        } else {
            midiOut = nullptr;
            std::cout << "\nMIDI Disabled" << std::endl;
        }

    } else if (midiOutSwitch && (midiOutputs.size() == 0)) {
        midiOut = nullptr; // if midi route switch off, or there is no MIDI device is available, set to null
        std::cout << "\nUNSUCCESSFUL: No Midi Port" << std::endl;
    } else {
        midiOut = nullptr;
        std::cout << "\nMIDI Switch Off" << std::endl;
    }

    //sfizz load into ram
    loadDrumSound("Instruments/SMDrums_Sforzando_1.2/Programs/SM_Drums_kit.sfz");
    //reserved load for keyboard

    synth.addVoice(new SineWaveVoice());
    synth.addSound(new SineWaveSound());

    deviceManager.initialise(0, 2, nullptr, true);

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);

    setup.bufferSize = 256;   // or 512 if 256 is unstable
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
    synth.setCurrentPlaybackSampleRate (device->getCurrentSampleRate());

    drumSynth.setSampleRate(device->getCurrentSampleRate());
    drumSynth.setSamplesPerBlock(device->getCurrentBufferSizeSamples());
}

void HeadlessAudioEngine::audioDeviceStopped() {}

void HeadlessAudioEngine::loadDrumSound(const juce::String& sfzPath) 
{
    bool loaded = drumSynth.loadSfzFile(sfzPath.toStdString());
    if (!loaded) {
        std::cerr << "ERROR: Failed to load SFZ: " << sfzPath << std::endl;
        // set a flag so you can skip drumSynth.renderBlock entirely
    }
}

void HeadlessAudioEngine::audioDeviceIOCallbackWithContext(
    const float* const*, int, float* const* outputChannelData, int numOutputChannels,
    int numSamples, const juce::AudioIODeviceCallbackContext&) 
    {
    juce::AudioBuffer<float> buffer (const_cast<float**> (outputChannelData), numOutputChannels, numSamples);

    buffer.clear();

    // Current instrument picked
    auto activeInst = globalState->currentInstrument.load();

    if (activeInst == ActiveInstrument::Theremin) {
        // READ HAND VISIBILITY
        bool isRightVisible = globalState->rightHandVisible.load();
        bool isLeftVisible = globalState->leftHandVisible.load();
        
        // if right visible and was not previously visible, turn note on
        if (isRightVisible && !wasRightVisible) {
            synth.noteOn(1, 60, 1.0f); //base note to wake voice up
            if (midiOut != nullptr) {
            // only send midi message if switch is on and a MIDI device was opened
                midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, 60, 1.0f));
            }
        // if right is not visible and was previously visible, turn note off
        } else if (!isRightVisible && wasRightVisible) {
            synth.noteOff(1, 60, 1.0f, true); 
            if (midiOut != nullptr) {
                midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, 60, 0.0f));
            }
        }
        wasRightVisible = isRightVisible;

        // 3. Continuous Theremin Math
        if (isRightVisible) {
            float x = globalState->rightHandX.load();
            float y = globalState->leftHandY.load();

            // 1. Map your X hand (0.0 to 1.0) to represent -24 to +24 semitones.
            //    Center (0.5) = 0 bend. Left (0.0) = -24. Right (1.0) = +24.
            float semitonesFromCenter = (x * 48.0f) - 24.0f;
            
            // 2. Calculate the exact Hz using the musical pitch formula.
            //    (Base Note = Middle C = 261.625 Hz)
            double targetFreq = 261.625565 * std::pow(2.0, semitonesFromCenter / 12.0);

            // Left Hand Volume: Invert Y so up is loud, down is quiet.
            // If left hand isn't visible, volume forces to 0.0f.
            float safeY = juce::jlimit(0.0f, 1.0f, y); //mediapipe goes past 1 as an estimate
            float targetVol = isLeftVisible ? (1.0f - safeY) : 0.0f; 

            // 4. Cast the Voice and push the math directly into it
            if (auto* myVoice = dynamic_cast<SineWaveVoice*>(synth.getVoice(0))) {
                myVoice->setWaveform(globalState->currentWaveform.load());
                myVoice->updateThereminMath(targetFreq, targetVol);
            }

            if (midiOut != nullptr) {
                // Convert 0.0-1.0 floats to 0-127 MIDI values and pitch bend value
                int midiPitchBend = static_cast<int>(x * 16383.0f);
                int midiVolume = static_cast<int>(targetVol * 127.0f);

                // Send X to MIDI CC 1 (Modulation Wheel)
                midiOut->sendMessageNow(juce::MidiMessage::pitchWheel(1, midiPitchBend));
                
                // Send Y to MIDI CC 7 (Volume)
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
            if (midiOut != nullptr) midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, leftNote, (juce::uint8)leftVelocity));
        }

        if (globalState->rightDrumHit.exchange(false)) {
            int rightNote = globalState->rightDrumType.load();
            int rightVelocity = globalState->rightDrumVelocity.load();

            drumSynth.noteOn(0, rightNote, rightVelocity);
            if (midiOut != nullptr) midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, rightNote, (juce::uint8)rightVelocity));
        }

        //render drumhit
        float* outChannels[] = { buffer.getWritePointer(0), buffer.getWritePointer(1) };
        drumSynth.renderBlock(outChannels, numSamples);
    }
}
