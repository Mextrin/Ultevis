#include "AppEngine.h"
#include "../Core/GlobalState.h"
#include "../Audio/AudioEngine.h"

#include <QCoreApplication>
#include <QtGlobal>

// --- Network headers for sendCommandToPython ---
#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

namespace {
    void sendCommandToPython(const std::string& mode, bool quit = false) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(5006); 
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        std::string json = "{\"mode\": \"" + mode + "\", \"quit\": " + (quit ? "true" : "false") + "}";
        sendto(sock, json.c_str(), json.length(), 0, (sockaddr*)&addr, sizeof(addr));

        #ifdef _WIN32
            closesocket(sock);
        #else
            close(sock);
        #endif
    }
}

namespace airchestra {

AppEngine::AppEngine(GlobalState* gState, HeadlessAudioEngine* aEngine, QObject *parent) 
    : QObject(parent), globalState(gState), audioEngine(aEngine) 
{
    logger.log(AppEventType::AppStarted, {{"mode", "interactive"}});
    cameraPermissionStatus = "granted";

    midiDeviceNames.append("None");
    midiDeviceIds.push_back("");
    for (const auto& [id, name] : audioEngine->getAvailableMidiDevices()) {
        midiDeviceNames.append(QString::fromStdString(name));
        midiDeviceIds.push_back(id);
    }

    refreshTrackedState();
    connect(&handStatePollTimer, &QTimer::timeout, this, &AppEngine::refreshTrackedState);
    handStatePollTimer.start(33);

    sendCommandToPython("none");
}

AppEngine::~AppEngine() {}

void AppEngine::setCameraPermissionStatus(const QString &value) {
    if (cameraPermissionStatus == value) return;
    cameraPermissionStatus = value;
    emit cameraPermissionChanged();
}

void AppEngine::requestCameraPermission() {
    setCameraPermissionStatus("granted");
}

void AppEngine::proceed() {
    state.setCurrentScreen(static_cast<int>(AppScreen::Session));
    state.setSessionRunning(true);
}

void AppEngine::goBack() {
    for (int i = 0; i < 128; ++i) {
        globalState->keyboardState[i].store(false);
    }
    state.setCurrentScreen(static_cast<int>(AppScreen::Session));

    sendCommandToPython("none");
}

void AppEngine::selectInstrument(const QString &name) {
    if (name == QStringLiteral("theremin")) {
        state.setCurrentScreen(static_cast<int>(AppScreen::Theremin));
        globalState->currentInstrument.store(ActiveInstrument::Theremin);
        sendCommandToPython("theremin");
    }
    else if (name == QStringLiteral("drums")) {
        state.setCurrentScreen(static_cast<int>(AppScreen::Drums)); 
        globalState->currentInstrument.store(ActiveInstrument::Drums);
        audioEngine->loadDrumSound("Instruments/SMDrums_Sforzando_1.2/Programs/SM_Drums_kit.sfz");
        sendCommandToPython("drums");
    }
    else if (name == QStringLiteral("keyboard")) {
        state.setCurrentScreen(static_cast<int>(AppScreen::Keyboard));
        globalState->currentInstrument.store(ActiveInstrument::Keyboard);
        globalState->currentKeyboardInstrument.store(KeyboardSound::GrandPiano);
        audioEngine->loadKeyboardSound(0);
        sendCommandToPython("keyboard");
    } 
}

// ---------------------------------------------------------------------------
// Settings routed safely to GlobalState
// ---------------------------------------------------------------------------

void AppEngine::setMidiEnabled(bool enabled) {
    globalState->routeToMidiOut.store(enabled);
}

void AppEngine::selectMidiDevice(const QString& displayName) {
    if (m_currentMidiDevice != displayName) {
        m_currentMidiDevice = displayName;
        emit currentMidiDeviceChanged();
    }

    // If the user selects "None", close the port and disable routing
    if (displayName == "None" || displayName.isEmpty()) {
        audioEngine->openMidiDevice("");
        globalState->routeToMidiOut.store(false);
        return;
    }

    // Search our list of detected devices for the exact name
    for (int i = 1; i < midiDeviceNames.size(); ++i) {
        if (midiDeviceNames[i] == displayName) {
            // ACTUALLY TELL JUCE TO OPEN THE PORT
            audioEngine->openMidiDevice(midiDeviceIds[static_cast<size_t>(i)]);
            globalState->routeToMidiOut.store(true);
            return;
        }
    }
    
    // Fallback if something goes wrong
    audioEngine->openMidiDevice("");
    globalState->routeToMidiOut.store(false);
}

void AppEngine::setMasterVolume(float v) {
    globalState->masterVolume.store(qBound(0.0f, v, 1.0f));
    state.setMasterVolume(v);
}

void AppEngine::setMouthKickEnabled(bool enabled) {
    globalState->mouthKickEnable.store(enabled);
    state.setMouthKickEnabled(enabled);
}

void AppEngine::setThereminWaveform(const QString& wave) {
    Waveform wf = Waveform::Sine;
    if (wave == "square")        wf = Waveform::Square;
    else if (wave == "sawtooth") wf = Waveform::Saw;
    else if (wave == "triangle") wf = Waveform::Triangle;
    globalState->currentWaveform.store(wf);
}

void AppEngine::setThereminSemitoneRangeOneSide(int semitones) {
    globalState->thereminSemitoneRangeOneSide.store(qBound(12, semitones, 96));
    state.setThereminSemitoneRangeOneSide(semitones);
}

void AppEngine::setThereminCenterNote(int midiNote) {
    globalState->thereminCenterNote.store(qBound(24, midiNote, 96));
    state.setThereminCenterNote(midiNote);
}

void AppEngine::setThereminVolumeFloor(float v) {
    globalState->thereminVolumeFloor.store(qBound(0.0f, v, 1.0f));
    state.setThereminVolumeFloor(v);
}

void AppEngine::triggerDrumHit(int midiNote, int velocity) {
    globalState->rightDrumType.store(midiNote);
    globalState->rightDrumVelocity.store(qBound(0, velocity, 127));
    globalState->rightDrumHit.store(true);
}

void AppEngine::triggerKeyboardNote(int midiNote, int velocity) {
    if (midiNote >= 0 && midiNote < 128) {
        globalState->keyboardState[midiNote].store(true);
    }
}

void AppEngine::releaseKeyboardNote(int midiNote) { 
    if (midiNote >= 0 && midiNote < 128) {
        globalState->keyboardState[midiNote].store(false);
    }
}

void AppEngine::adjustKeyboardOctave(int keyboardIndex, int delta) {
    if (globalState == nullptr || delta == 0)
        return;

    if (keyboardIndex == 1) {
        const int current = globalState->topKeyboardOctave.load();
        const int next = qBound(0, current + delta, 8);
        if (next == current)
            return;

        globalState->topKeyboardOctave.store(next);
        refreshTrackedState();
        return;
    }

    if (keyboardIndex == 2) {
        const int current = globalState->bottomKeyboardOctave.load();
        const int next = qBound(0, current + delta, 8);
        if (next == current)
            return;

        globalState->bottomKeyboardOctave.store(next);
        refreshTrackedState();
    }
}

void AppEngine::setSustainPedal(bool enabled) {
    globalState->sustainPedal.store(enabled);
    state.setSustainPedal(enabled);
}

void AppEngine::refreshTrackedState() {
    if (globalState == nullptr)
        return;

    const bool nextLeftHandVisible = globalState->leftHandVisible.load();
    const bool nextRightHandVisible = globalState->rightHandVisible.load();
    const qreal nextLeftHandX = globalState->leftHandX.load();
    const qreal nextLeftHandY = globalState->leftHandY.load();
    const qreal nextRightHandX = globalState->rightHandX.load();
    const qreal nextRightHandY = globalState->rightHandY.load();
    const bool nextLeftPinch = globalState->leftPinch.load();
    const bool nextRightPinch = globalState->rightPinch.load();

    const bool handChanged =
        m_leftHandVisible != nextLeftHandVisible ||
        m_rightHandVisible != nextRightHandVisible ||
        m_leftHandX != nextLeftHandX ||
        m_leftHandY != nextLeftHandY ||
        m_rightHandX != nextRightHandX ||
        m_rightHandY != nextRightHandY ||
        m_leftPinch != nextLeftPinch ||
        m_rightPinch != nextRightPinch;

    m_leftHandVisible = nextLeftHandVisible;
    m_rightHandVisible = nextRightHandVisible;
    m_leftHandX = nextLeftHandX;
    m_leftHandY = nextLeftHandY;
    m_rightHandX = nextRightHandX;
    m_rightHandY = nextRightHandY;
    m_leftPinch = nextLeftPinch;
    m_rightPinch = nextRightPinch;

    if (handChanged)
        emit handStateChanged();

    const int nextTopKeyboardOctave = globalState->topKeyboardOctave.load();
    const int nextBottomKeyboardOctave = globalState->bottomKeyboardOctave.load();
    const bool octavesChanged =
        m_topKeyboardOctave != nextTopKeyboardOctave ||
        m_bottomKeyboardOctave != nextBottomKeyboardOctave;

    m_topKeyboardOctave = nextTopKeyboardOctave;
    m_bottomKeyboardOctave = nextBottomKeyboardOctave;

    if (octavesChanged)
        emit keyboardOctavesChanged();
}

} // namespace airchestra
