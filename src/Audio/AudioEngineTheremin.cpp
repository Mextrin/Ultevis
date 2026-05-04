#include "AudioEngine.h"
#include <cmath>

// Processes the theremin
void HeadlessAudioEngine::processTheremin(juce::AudioBuffer<float>& buffer, int numSamples)
{
    const bool isRightVisible = globalState->rightHandVisible.load();
    const bool isLeftVisible = globalState->leftHandVisible.load();

    // We use a static variable to remember exactly which note we turned ON,
    // so we can turn the correct note OFF even if the user moves the slider mid-note.
    static int activeThereminNote = 60;

    if (isRightVisible && !wasRightVisible) {
        // Read the live UI slider value instead of the hardcoded 60
        activeThereminNote = globalState->thereminCenterNote.load();
        
        synth.noteOn(1, activeThereminNote, 1.0f);
        if (midiOut != nullptr) {
            midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, activeThereminNote, 1.0f));
        }
    }
    else if (!isRightVisible && wasRightVisible) {
        // Turn off the exact note we started
        synth.noteOff(1, activeThereminNote, 1.0f, true);
        if (midiOut != nullptr) {
            midiOut->sendMessageNow(juce::MidiMessage::noteOff(1, activeThereminNote, 0.0f));
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

        const float semitonesFromCenter = (normalizedX * (semitoneRangeOneSide*2.0f)) - (semitoneRangeOneSide);
        const float targetMidiNote = centerMidiNote + semitonesFromCenter;
        const double targetFreq = 440.0f * std::pow(2.0f, (targetMidiNote - 69.0f) / 12.0f);

        const float safeY = juce::jlimit(0.0f, 1.0f, y);
        const float volFloor = globalState->thereminVolumeFloor.load();
        const float targetVol = isLeftVisible ? (volFloor + (1.0f - volFloor) * (1.0f - safeY)) : 0.0f;

        if (auto* myVoice = dynamic_cast<SineWaveVoice*>(synth.getVoice(0))) {
            myVoice->setWaveform(globalState->currentWaveform.load());
            myVoice->updateThereminMath(targetFreq, targetVol);
        }

        if (midiOut != nullptr) {
            // Pitch bend maps 0.0 -> 1.0 to 0 -> 16383
            int midiPitchBend = static_cast<int>(normalizedX * 16383.0f);
            int midiVolume = static_cast<int>(targetVol * 127.0f);

            midiOut->sendMessageNow(juce::MidiMessage::pitchWheel(1, midiPitchBend));
            midiOut->sendMessageNow(juce::MidiMessage::controllerEvent(1, 7, midiVolume));
        }
    }

    synth.renderNextBlock(buffer, juce::MidiBuffer(), 0, numSamples);
}