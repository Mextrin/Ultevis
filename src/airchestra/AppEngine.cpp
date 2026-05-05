#include "AppEngine.h"
#include "../Core/GlobalState.h"
#include "../Audio/AudioEngine.h"

#include <QCoreApplication>
#include <QtGlobal>

#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QStringList>
#include <QFileInfo>

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
        #ifdef _WIN32
            static bool wsa_initialized = false;
            if (!wsa_initialized) {
                WSADATA wsa_data;
                WSAStartup(MAKEWORD(2, 2), &wsa_data);
                wsa_initialized = true;
            }
            SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock == INVALID_SOCKET) return;
        #else
            int sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock < 0) return;
        #endif

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

    // --- Helper function to trim drum SFZ file ---
    QString createTrimmedDrumSfz(const QString& originalSfzPath) {
        QFile originalFile(originalSfzPath);
        if (!originalFile.open(QIODevice::ReadOnly | QIODevice::Text)) return originalSfzPath;

        QFileInfo fileInfo(originalSfzPath);
        QString tempPath = fileInfo.absolutePath() + "/airchestra_lite_drums.sfz";
        
        QFile tempFile(tempPath);
        if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) return originalSfzPath;

        QTextStream in(&originalFile);
        QTextStream out(&tempFile);

        while (!in.atEnd()) {
            QString line = in.readLine();
            
            if (line.trimmed().startsWith("#include")) {
                
                if (!(line.contains("kick") || 
                      line.contains("snare") || 
                      line.contains("tom") || 
                      line.contains("hat") || 
                      line.contains("crash") || 
                      line.contains("ride") || 
                      line.contains("curves"))) 
                {
                    continue;
                }
            }

            out << line << "\n";
        }

        originalFile.close();
        tempFile.close();
        
        return tempPath; 
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
        globalState->keyboardNoteVelocity[i].store(100);
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
        
        QString liteSfz = createTrimmedDrumSfz("Instruments/SMDrums_Sforzando_1.2/Programs/SM_Drums_kit.sfz");
        
        audioEngine->loadDrumSound(liteSfz.toStdString()); 
        
        sendCommandToPython("drums");
    }
    else if (name == QStringLiteral("keyboard")) {
        state.setCurrentScreen(static_cast<int>(AppScreen::Keyboard));
        globalState->currentInstrument.store(ActiveInstrument::Keyboard);
        globalState->currentKeyboardInstrument.store(KeyboardSound::GrandPiano);
        audioEngine->loadKeyboardSound(0);
        sendCommandToPython("keyboard");
    }
    else if (name == QStringLiteral("guitar")) {
        globalState->currentInstrument.store(ActiveInstrument::Guitar);
        globalState->currentGuitarSound.store(GuitarSound::Acoustic);

        audioEngine->loadGuitarSound(2);

        // simple rotating test progression
        static int chordIndex = 0;

        switch (chordIndex % 6) {
            case 0: // C major
                globalState->currentGuitarRoot.store(GuitarChordRoot::E);
                globalState->currentGuitarQuality.store(GuitarChordQuality::Minor);
                break;

            case 1: // G major
                globalState->currentGuitarRoot.store(GuitarChordRoot::G);
                globalState->currentGuitarQuality.store(GuitarChordQuality::Major);
                break;

            case 2: // A minor
                globalState->currentGuitarRoot.store(GuitarChordRoot::A);
                globalState->currentGuitarQuality.store(GuitarChordQuality::Minor);
                break;

            case 3: // F major
                globalState->currentGuitarRoot.store(GuitarChordRoot::F);
                globalState->currentGuitarQuality.store(GuitarChordQuality::Major);
                break;

            case 4: // D minor
                globalState->currentGuitarRoot.store(GuitarChordRoot::D);
                globalState->currentGuitarQuality.store(GuitarChordQuality::Minor);
                break;

            case 5: // G7
                globalState->currentGuitarRoot.store(GuitarChordRoot::G);
                globalState->currentGuitarQuality.store(GuitarChordQuality::Dom7);
                break;
        }

        chordIndex++;

        globalState->guitarVelocity.store(110);
        globalState->guitarStrumHit.store(true);

        sendCommandToPython("guitar");
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

    if (displayName == "None" || displayName.isEmpty()) {
        audioEngine->openMidiDevice("");
        globalState->routeToMidiOut.store(false);
        return;
    }

    for (int i = 1; i < midiDeviceNames.size(); ++i) {
        if (midiDeviceNames[i] == displayName) {
            audioEngine->openMidiDevice(midiDeviceIds[static_cast<size_t>(i)]);
            globalState->routeToMidiOut.store(true);
            return;
        }
    }
    
    audioEngine->openMidiDevice("");
    globalState->routeToMidiOut.store(false);
}

void AppEngine::setMasterVolume(float v) {
    globalState->masterVolume.store(qBound(0.0f, v, 1.0f));
    state.setMasterVolume(v);
}

void AppEngine::setLeftDrumVelocity(int v) {
    const int clamped = qBound(0, v, 100);
    globalState->leftDrumVelocity.store(clamped);
    state.setLeftDrumVelocity(clamped);
}

void AppEngine::setRightDrumVelocity(int v) {
    const int clamped = qBound(0, v, 100);
    globalState->rightDrumVelocity.store(clamped);
    state.setRightDrumVelocity(clamped);
}

void AppEngine::setLeftKeyboardVelocity(int v) {
    const int clamped = qBound(0, v, 100);
    globalState->leftKeyboardVelocity.store(clamped);
    state.setLeftKeyboardVelocity(clamped);
}

void AppEngine::setRightKeyboardVelocity(int v) {
    const int clamped = qBound(0, v, 100);
    globalState->rightKeyboardVelocity.store(clamped);
    state.setRightKeyboardVelocity(clamped);
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
    (void)velocity;
    globalState->rightDrumType.store(midiNote);
    globalState->rightDrumHit.store(true);
}

void AppEngine::triggerKeyboardNote(int midiNote, int velocity) {
    if (midiNote >= 0 && midiNote < 128) {
        globalState->keyboardNoteVelocity[midiNote].store(qBound(1, velocity, 127));
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
        int minOctave = 1, maxOctave = 8;
        switch (globalState->currentKeyboardInstrument.load())
        {
            case KeyboardSound::GrandPiano: minOctave = 1; maxOctave = 7; break;
            case KeyboardSound::Organ:      minOctave = 2; maxOctave = 6; break;
            case KeyboardSound::Flute:      minOctave = 4; maxOctave = 6; break;
            case KeyboardSound::Harp:       minOctave = 1; maxOctave = 7; break;
            case KeyboardSound::Violin:     minOctave = 4; maxOctave = 6; break;
        }
        const int next = qBound(minOctave, current + delta, maxOctave);
        if (next == current)
            return;

        globalState->topKeyboardOctave.store(next);
        refreshTrackedState();
        return;
    }

    if (keyboardIndex == 2) {
        const int current = globalState->bottomKeyboardOctave.load();
        int minOctave = 1, maxOctave = 8;
        switch (globalState->currentKeyboardInstrument.load())
        {
            case KeyboardSound::GrandPiano: minOctave = 1; maxOctave = 7; break;
            case KeyboardSound::Organ:      minOctave = 2; maxOctave = 6; break;
            case KeyboardSound::Flute:      minOctave = 4; maxOctave = 6; break;
            case KeyboardSound::Harp:       minOctave = 1; maxOctave = 7; break;
            case KeyboardSound::Violin:     minOctave = 4; maxOctave = 6; break;
        }
        const int next = qBound(minOctave, current + delta, maxOctave);
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

void AppEngine::setKeyboardInstrument(int instrumentID) {
    globalState->currentKeyboardInstrument.store(static_cast<KeyboardSound>(instrumentID));
    audioEngine->loadKeyboardSound(instrumentID);

    int minOctave = 1, maxOctave = 8;
    switch (static_cast<KeyboardSound>(instrumentID))
    {
        case KeyboardSound::GrandPiano: minOctave = 1; maxOctave = 7; break;
        case KeyboardSound::Organ:      minOctave = 2; maxOctave = 6; break;
        case KeyboardSound::Flute:      minOctave = 4; maxOctave = 6; break;
        case KeyboardSound::Harp:       minOctave = 1; maxOctave = 7; break;
        case KeyboardSound::Violin:     minOctave = 4; maxOctave = 6; break;
    }

    globalState->topKeyboardOctave.store(
        qBound(minOctave, globalState->topKeyboardOctave.load(), maxOctave));
    globalState->bottomKeyboardOctave.store(
        qBound(minOctave, globalState->bottomKeyboardOctave.load(), maxOctave));

    refreshTrackedState();
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
    const bool nextLeftThumbUp = globalState->leftThumbUp.load();
    const bool nextLeftThumbDown = globalState->leftThumbDown.load();
    const bool nextRightThumbUp = globalState->rightThumbUp.load();
    const bool nextRightThumbDown = globalState->rightThumbDown.load();

    const bool handChanged =
        m_leftHandVisible != nextLeftHandVisible ||
        m_rightHandVisible != nextRightHandVisible ||
        m_leftHandX != nextLeftHandX ||
        m_leftHandY != nextLeftHandY ||
        m_rightHandX != nextRightHandX ||
        m_rightHandY != nextRightHandY ||
        m_leftPinch != nextLeftPinch ||
        m_rightPinch != nextRightPinch ||
        m_leftThumbUp != nextLeftThumbUp ||
        m_leftThumbDown != nextLeftThumbDown ||
        m_rightThumbUp != nextRightThumbUp ||
        m_rightThumbDown != nextRightThumbDown;

    m_leftHandVisible = nextLeftHandVisible;
    m_rightHandVisible = nextRightHandVisible;
    m_leftHandX = nextLeftHandX;
    m_leftHandY = nextLeftHandY;
    m_rightHandX = nextRightHandX;
    m_rightHandY = nextRightHandY;
    m_leftPinch = nextLeftPinch;
    m_rightPinch = nextRightPinch;
    m_leftThumbUp = nextLeftThumbUp;
    m_leftThumbDown = nextLeftThumbDown;
    m_rightThumbUp = nextRightThumbUp;
    m_rightThumbDown = nextRightThumbDown;

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

void AppEngine::setGuitarSound(int soundID) {
    globalState->currentGuitarSound.store(static_cast<GuitarSound>(soundID));
    audioEngine->loadGuitarSound(soundID);
}

void AppEngine::triggerGuitarStrum(int velocity) {
    globalState->guitarVelocity.store(qBound(0, velocity, 127));
    globalState->guitarStrumHit.store(true);
}

} // namespace airchestra