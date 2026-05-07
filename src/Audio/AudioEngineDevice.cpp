#include "AudioEngine.h"

void HeadlessAudioEngine::changeListenerCallback(juce::ChangeBroadcaster*)
{
    if (!isReinitializing)
        startTimer(500);
}

void HeadlessAudioEngine::timerCallback()
{
    stopTimer();
    isReinitializing = true;

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);

#if JUCE_MAC
    AudioDeviceID defaultDeviceID = 0;
    UInt32 propSize = sizeof(defaultDeviceID);
    AudioObjectPropertyAddress defaultAddr {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &defaultAddr, 0, nullptr, &propSize, &defaultDeviceID);

    CFStringRef cfName = nullptr;
    propSize = sizeof(cfName);
    AudioObjectPropertyAddress nameAddr {
        kAudioObjectPropertyName,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    AudioObjectGetPropertyData(defaultDeviceID, &nameAddr, 0, nullptr, &propSize, &cfName);
    if (cfName != nullptr)
    {
        setup.outputDeviceName = juce::String::fromCFString(cfName);
        CFRelease(cfName);
    }
#endif

    setup.useDefaultOutputChannels = true;
    deviceManager.setAudioDeviceSetup(setup, true);

    isReinitializing = false;
}