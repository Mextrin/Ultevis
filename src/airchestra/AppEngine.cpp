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
    globalState->isKeyPressed.store(false);  // release keyboard notes
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
        audioEngine->loadDrumSound("/Users/alexrystrom/Documents/GitHub/Ultevis/Instruments/SMDrums_Sforzando_1.2/Programs/SM_Drums_kit.sfz");
        sendCommandToPython("drums");
    }
    else if (name == QStringLiteral("keyboard")) {
        // Temporarily map to Drums screen index if Keyboard isn't in ViewState AppScreen enum yet
        state.setCurrentScreen(static_cast<int>(AppScreen::Session)); 
        globalState->currentInstrument.store(ActiveInstrument::Keyboard);
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
}

void AppEngine::setThereminWaveform(const QString& wave) {
    Waveform wf = Waveform::Sine;
    if (wave == "square")        wf = Waveform::Square;
    else if (wave == "sawtooth") wf = Waveform::Saw;
    else if (wave == "triangle") wf = Waveform::Triangle;
    globalState->currentWaveform.store(wf);
}

void AppEngine::setThereminSemitoneRange(int semitones) {
    globalState->thereminSemitoneRangeOneSide.store(qBound(12, semitones, 96));
    state.setThereminSemitoneRange(semitones);
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
    globalState->keyboardNote.store(midiNote);
    globalState->keyboardVelocity.store(qBound(0, velocity, 127));
    globalState->isKeyPressed.store(true);
}

void AppEngine::releaseKeyboardNote() {
    globalState->isKeyPressed.store(false);
}

} // namespace airchestra