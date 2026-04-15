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
   #ifdef _WIN32
    // Keep a shorter smoothing window on Windows so pitch and volume react faster.
    smoothedFrequency.reset(getSampleRate(), 0.01);
    smoothedVolume.reset(getSampleRate(), 0.01);
   #else
    // Setup the smoothers to interpolate over 20 milliseconds (glitch prevention)
    smoothedFrequency.reset(getSampleRate(), 0.02);
    smoothedVolume.reset(getSampleRate(), 0.02);
   #endif
    
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

void SineWaveVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) 
{
    for (int i = 0; i < numSamples; ++i)
    {
        // 1. Advance the smoothers
        double currentFreq = smoothedFrequency.getNextValue();
        float currentVol = smoothedVolume.getNextValue();
        float currentAdsr = adsr.getNextSample();

        // 2. Calculate the sine wave phase
        auto cyclesPerSample = currentFreq / getSampleRate();
        auto angleDelta = cyclesPerSample * juce::MathConstants<double>::twoPi;
        
        currentAngle += angleDelta;
        if (currentAngle > juce::MathConstants<double>::twoPi)
            currentAngle -= juce::MathConstants<double>::twoPi;

        // 3. Generate the final sound (Sine * ADSR * Left Hand Volume)
        // Multiply by 0.2f as a master safety volume so you don't blow the speakers
        float currentSample = (float) std::sin(currentAngle) * currentAdsr * currentVol * 0.2f;

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

    synth.addVoice(new SineWaveVoice());
    synth.addSound(new SineWaveSound());

    deviceManager.initialiseWithDefaultDevices(0, 2);
    deviceManager.addAudioCallback(this);
}

HeadlessAudioEngine::~HeadlessAudioEngine() {
    deviceManager.removeAudioCallback(this);
}

void HeadlessAudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    synth.setCurrentPlaybackSampleRate (device->getCurrentSampleRate());
}

void HeadlessAudioEngine::audioDeviceStopped() {}

void HeadlessAudioEngine::audioDeviceIOCallbackWithContext(
    const float* const*, int, float* const* outputChannelData, int numOutputChannels,
    int numSamples, const juce::AudioIODeviceCallbackContext&) 
{
    juce::AudioBuffer<float> buffer (const_cast<float**> (outputChannelData), numOutputChannels, numSamples);
    buffer.clear();

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
    if (isRightVisible) 
    {
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
        float targetVol = isLeftVisible ? (1.0f - y) : 0.0f; 

        // 4. Cast the Voice and push the math directly into it
        if (auto* myVoice = dynamic_cast<SineWaveVoice*>(synth.getVoice(0))) {
            myVoice->updateThereminMath(targetFreq, targetVol);
        }

        if (midiOut != nullptr) 
        {
            // Convert 0.0-1.0 floats to 0-127 MIDI values and pitch bend value
            int midiPitchBend = static_cast<int>(x * 16383.0f);
            int midiVolume = static_cast<int>(targetVol * 127.0f);

            // Send X to MIDI CC 1 (Modulation Wheel)
            midiOut->sendMessageNow(juce::MidiMessage::pitchWheel(1, midiPitchBend));
            
            // Send Y to MIDI CC 11 (Expression/Volume)
            midiOut->sendMessageNow(juce::MidiMessage::controllerEvent(1, 11, midiVolume));
        }
    }

    // 5. Render
    synth.renderNextBlock(buffer, juce::MidiBuffer(), 0, numSamples);
}
