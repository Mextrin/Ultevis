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
    bool midiOutSwitch = globalState->routeToMidiOut.load(); //load from globalstate
    auto midiOutputs = juce::MidiOutput::getAvailableDevices(); //gets all midi devices
    midiOut = juce::MidiOutput::openDevice(midiOutputs[0].identifier); //takes 1st device identifier
    if (!midiOutSwitch) {
        midiOut = nullptr; //if midiroute switch off, set to null
    }

    synth.addVoice (new SineWaveVoice()); // Add 1 voice (Monophonic Theremin)
    synth.addSound (new SineWaveSound());

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

    // 1. Read Lock-Free State from Pod 4 (Bridge)
    bool isRightVisible = globalState->rightHandVisible.load();
    bool isLeftVisible = globalState->leftHandVisible.load();
    
    // 2. Trigger Logic (Start/Stop the ADSR)
    if (isRightVisible && !wasRightVisible) {
        synth.noteOn(1, 60, 1.0f); // Base note just to wake up the Voice
        if (midiOut != nullptr) { //only send midi message if switch is on
            midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, 60, 1.0f));
        }
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

        // Right Hand Pitch: Scale 0.0-1.0 to 200Hz-1000Hz
        float targetFreq = 200.0f + (x * 800.0f);

        // Left Hand Volume: Invert Y so up is loud, down is quiet.
        // If left hand isn't visible, volume forces to 0.0f.
        float targetVol = isLeftVisible ? (1.0f - y) : 0.0f; 

        // 4. Cast the Voice and push the math directly into it
        if (auto* myVoice = dynamic_cast<SineWaveVoice*>(synth.getVoice(0))) {
            myVoice->updateThereminMath(targetFreq, targetVol);
        }

        if (midiOut != nullptr) 
        {
            // Convert 0.0-1.0 floats to 0-127 MIDI values
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