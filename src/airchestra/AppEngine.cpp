#include "AppEngine.h"

// JUCE + backend headers — only included here so the rest of the app
// never needs to pull in the full JUCE header tree.
#include "../Core/GlobalState.h"
#include "../Audio/AudioEngine.h"
#include "../Camera/CameraFeed.h"

#include <QCoreApplication>
#include <QPermissions>
#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QTimer>

namespace airchestra {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

AppEngine::AppEngine(QObject* parent) : QObject(parent)
{
    logger.log(AppEventType::AppStarted, {{"mode", "interactive"}});

    // Seed camera-permission status from current OS state
    QCameraPermission camPerm;
    switch (qApp->checkPermission(camPerm)) {
    case Qt::PermissionStatus::Granted:     cameraPermissionStatus = "granted";     break;
    case Qt::PermissionStatus::Denied:      cameraPermissionStatus = "denied";      break;
    case Qt::PermissionStatus::Undetermined:cameraPermissionStatus = "undetermined";break;
    }

    // --- Backend ---
    globalState  = std::make_unique<GlobalState>();
    audioEngine  = std::make_unique<HeadlessAudioEngine>(globalState.get());

    // --- MIDI device list (auto-detected via JUCE) ---
    midiDeviceNames.append("None");
    midiDeviceIds.push_back("");
    for (const auto& [id, name] : audioEngine->getAvailableMidiDevices()) {
        midiDeviceNames.append(QString::fromStdString(name));
        midiDeviceIds.push_back(id);
    }

    // --- Start UDP camera-feed listener thread ---
    startCameraThread();
}

AppEngine::~AppEngine()
{
    cameraStopFlag.store(true);
    if (cameraThread.joinable())
        cameraThread.join();

    stopHandDetector();
    // audioEngine destructor handles JUCE teardown
}

// ---------------------------------------------------------------------------
// Camera permission
// ---------------------------------------------------------------------------

void AppEngine::setCameraPermissionStatus(const QString& value)
{
    if (cameraPermissionStatus == value) return;
    cameraPermissionStatus = value;
    emit cameraPermissionChanged();
}

void AppEngine::requestCameraPermission()
{
    QCameraPermission camPerm;
    const auto status = qApp->checkPermission(camPerm);
    if (status == Qt::PermissionStatus::Granted) { setCameraPermissionStatus("granted");     return; }
    if (status == Qt::PermissionStatus::Denied)  { setCameraPermissionStatus("denied");      return; }

    qApp->requestPermission(camPerm, this, [this](const QPermission& result) {
        switch (result.status()) {
        case Qt::PermissionStatus::Granted:
            logger.log(AppEventType::SessionStateChanged, {{"camera_permission", "granted"}});
            setCameraPermissionStatus("granted");
            break;
        case Qt::PermissionStatus::Denied:
            logger.log(AppEventType::SessionStateChanged, {{"camera_permission", "denied"}});
            setCameraPermissionStatus("denied");
            break;
        case Qt::PermissionStatus::Undetermined:
            setCameraPermissionStatus("undetermined");
            break;
        }
    });
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

void AppEngine::proceed()
{
    logger.log(AppEventType::ButtonClicked, {{"button", "proceed"}});
    state.setCurrentScreen(static_cast<int>(AppScreen::Session));
    state.setSessionRunning(true);
    logger.log(AppEventType::ScreenChanged, {{"screen", "session"}});
}

void AppEngine::goBack()
{
    const auto current = static_cast<AppScreen>(state.currentScreen());

    // Stop gesture process when leaving any camera-driven instrument
    if (current == AppScreen::Theremin || current == AppScreen::Drums)
        stopHandDetector();

    switch (current) {
    case AppScreen::Theremin:
    case AppScreen::Drums:
        state.setCurrentScreen(static_cast<int>(AppScreen::Session));
        logger.log(AppEventType::ScreenChanged, {{"screen", "session"}});
        break;
    case AppScreen::Keyboard:
        globalState->isKeyPressed.store(false);  // release any held note
        state.setCurrentScreen(static_cast<int>(AppScreen::Session));
        logger.log(AppEventType::ScreenChanged, {{"screen", "session"}});
        break;
    case AppScreen::Guitar:
        state.setCurrentScreen(static_cast<int>(AppScreen::Session));
        logger.log(AppEventType::ScreenChanged, {{"screen", "session"}});
        break;
    default:
        state.setCurrentScreen(static_cast<int>(AppScreen::Landing));
        state.setSessionRunning(false);
        logger.log(AppEventType::ScreenChanged, {{"screen", "landing"}});
        break;
    }
}

void AppEngine::selectInstrument(const QString& name)
{
    logger.log(AppEventType::ButtonClicked, {{"button", "select_instrument"}, {"instrument", name}});

    if (name == QStringLiteral("theremin")) {
        globalState->currentInstrument.store(ActiveInstrument::Theremin);
        launchHandDetector("theremin");
        state.setCurrentScreen(static_cast<int>(AppScreen::Theremin));
        logger.log(AppEventType::ScreenChanged, {{"screen", "theremin"}});
    } else if (name == QStringLiteral("drums")) {
        globalState->currentInstrument.store(ActiveInstrument::Drums);
        launchHandDetector("drums");
        state.setCurrentScreen(static_cast<int>(AppScreen::Drums));
        logger.log(AppEventType::ScreenChanged, {{"screen", "drums"}});
    } else if (name == QStringLiteral("keyboard")) {
        globalState->currentInstrument.store(ActiveInstrument::Keyboard);
        state.setCurrentScreen(static_cast<int>(AppScreen::Keyboard));
        logger.log(AppEventType::ScreenChanged, {{"screen", "keyboard"}});
    } else if (name == QStringLiteral("guitar")) {
        state.setCurrentScreen(static_cast<int>(AppScreen::Guitar));
        logger.log(AppEventType::ScreenChanged, {{"screen", "guitar"}});
    }
}

// ---------------------------------------------------------------------------
// MIDI
// ---------------------------------------------------------------------------

void AppEngine::setMidiEnabled(bool enabled)
{
    state.setMidiEnabled(enabled);
    if (!enabled)
        audioEngine->openMidiDevice("");
    logger.log(AppEventType::SessionStateChanged, {{"midi_output", enabled ? "on" : "off"}});
}

void AppEngine::selectMidiDevice(const QString& displayName)
{
    if (displayName == "None" || displayName.isEmpty()) {
        audioEngine->openMidiDevice("");
        state.setMidiEnabled(false);
        return;
    }

    // Find the JUCE identifier that matches the display name
    for (int i = 1; i < midiDeviceNames.size(); ++i) {
        if (midiDeviceNames[i] == displayName) {
            audioEngine->openMidiDevice(midiDeviceIds[static_cast<size_t>(i)]);
            state.setMidiEnabled(true);
            logger.log(AppEventType::SessionStateChanged,
                       {{"midi_device", displayName}});
            return;
        }
    }
    // Name not found — disable MIDI
    audioEngine->openMidiDevice("");
    state.setMidiEnabled(false);
}

// ---------------------------------------------------------------------------
// Theremin settings — write straight to GlobalState for zero-latency pickup
// ---------------------------------------------------------------------------

void AppEngine::setMasterVolume(float v)
{
    const float clamped = qBound(0.0f, v, 1.0f);
    globalState->masterVolume.store(clamped);
    state.setMasterVolume(clamped);
}

void AppEngine::setThereminWaveform(const QString& wave)
{
    Waveform wf = Waveform::Sine;
    if (wave == "square")   wf = Waveform::Square;
    else if (wave == "sawtooth") wf = Waveform::Saw;
    else if (wave == "triangle") wf = Waveform::Triangle;
    globalState->currentWaveform.store(wf);
    state.setThereminWaveform(wave);
}

void AppEngine::setThereminFreqMin(float hz)
{
    globalState->freqMin.store(hz);
    state.setThereminFreqMin(hz);
}

void AppEngine::setThereminFreqMax(float hz)
{
    globalState->freqMax.store(hz);
    state.setThereminFreqMax(hz);
}

void AppEngine::setThereminVolumeFloor(float v)
{
    globalState->volumeFloor.store(qBound(0.0f, v, 1.0f));
    state.setThereminVolumeFloor(v);
}

void AppEngine::triggerDrumHit(int midiNote, int velocity)
{
    globalState->rightDrumType.store(midiNote);
    globalState->rightDrumVelocity.store(qBound(0, velocity, 127));
    globalState->rightDrumHit.store(true);
}

void AppEngine::triggerKeyboardNote(int midiNote, int velocity)
{
    globalState->keyboardNote.store(midiNote);
    globalState->keyboardVelocity.store(qBound(0, velocity, 127));
    globalState->isKeyPressed.store(true);
}

void AppEngine::releaseKeyboardNote()
{
    globalState->isKeyPressed.store(false);
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void AppEngine::startCameraThread()
{
    cameraStopFlag.store(false);
    cameraThread = std::thread([this]() {
        startCameraFeed(globalState.get(), &cameraStopFlag);
    });
}

void AppEngine::launchHandDetector(const QString& mode)
{
    // Guard: don't restart if the same mode is already running.
    if (handDetectorProcess && handDetectorProcess->state() == QProcess::Running
            && currentDetectorMode == mode)
        return;

    stopHandDetector();

    // appDir = <project>/build/Airchestra.app/Contents/MacOS
    // Four levels up lands at the project root regardless of how the app was launched.
    const QString appDir     = QCoreApplication::applicationDirPath();
    const QString projectRoot = QDir::cleanPath(appDir + "/../../../../");

    qDebug() << "AppEngine: appDir      =" << appDir;
    qDebug() << "AppEngine: projectRoot =" << projectRoot;

    // --- Resolve hand_detector.py ---
    QStringList scriptCandidates = {
        projectRoot + "/src/mediapipe/hand_detector.py",   // normal dev layout
        appDir      + "/../Resources/hand_detector.py",    // bundled copy
        appDir      + "/hand_detector.py",                 // beside executable
    };
    QString scriptPath;
    for (const auto& c : scriptCandidates) {
        const QString clean = QDir::cleanPath(c);
        qDebug() << "AppEngine: checking script:" << clean << "exists:" << QFileInfo::exists(clean);
        if (QFileInfo::exists(clean)) { scriptPath = clean; break; }
    }
    if (scriptPath.isEmpty()) {
        qWarning("AppEngine: hand_detector.py not found — gesture input disabled");
        return;
    }
    qDebug() << "AppEngine: using script:" << scriptPath;

    // --- Resolve python3 executable ---
    // macOS app bundles launch with a stripped PATH that omits Homebrew and the
    // project venv.  Search known locations so the subprocess can actually start.
    QStringList pythonCandidates = {
        projectRoot + "/venv/bin/python3",   // project venv (preferred)
        "/opt/homebrew/bin/python3",         // Homebrew Apple Silicon
        "/usr/local/bin/python3",            // Homebrew Intel / old layout
        "/usr/bin/python3",                  // Xcode CLT system Python
    };
    QString python;
    for (const auto& p : pythonCandidates) {
        const QString clean = QDir::cleanPath(p);
        qDebug() << "AppEngine: checking python:" << clean << "exists:" << QFileInfo::exists(clean);
        if (QFileInfo::exists(clean)) { python = clean; break; }
    }
    if (python.isEmpty()) {
        qWarning("AppEngine: python3 not found — gesture input disabled.\n"
                 "  Fix: cd <project> && python3 -m venv venv && "
                 "source venv/bin/activate && pip install mediapipe opencv-python");
        return;
    }
    qDebug() << "AppEngine: using python:" << python;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("ULTEVIS_CAMERA_MODE", mode);
    env.insert("ULTEVIS_HEADLESS", "1"); // camera is shown in the Qt UI, not a separate OpenCV window

    handDetectorProcess = new QProcess(this);
    handDetectorProcess->setProcessEnvironment(env);
    handDetectorProcess->setWorkingDirectory(projectRoot);

    // Python writes a single 'R' byte when it has finished processing a frame
    // and is ready for the next one (pull protocol — prevents stale-frame queuing).
    connect(handDetectorProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        const QByteArray out = handDetectorProcess->readAllStandardOutput();
        if (out.contains('R'))
            pipeNextFrame();
        const QByteArray rest = QByteArray(out).replace('R', "").trimmed();
        if (!rest.isEmpty())
            qDebug() << "[hand_detector]" << rest;
    });
    connect(handDetectorProcess, &QProcess::readyReadStandardError, this, [this]() {
        qWarning() << "[hand_detector stderr]" << handDetectorProcess->readAllStandardError().trimmed();
    });

    connect(handDetectorProcess, &QProcess::stateChanged,
            this, [this](QProcess::ProcessState s) {
        const bool running = (s == QProcess::Running);
        if (running != handDetectorIsRunning) {
            handDetectorIsRunning = running;
            emit handDetectorRunningChanged();
        }
    });

    connect(handDetectorProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus status) {
        qWarning() << "AppEngine: hand_detector exited, code =" << code
                   << "status =" << (status == QProcess::NormalExit ? "normal" : "crash");
        // Auto-restart if we still need gesture input (not deliberately stopped).
        if (!currentDetectorMode.isEmpty())
            QTimer::singleShot(500, this, [this]() { launchHandDetector(currentDetectorMode); });
    });

    connect(handDetectorProcess, &QProcess::errorOccurred,
            this, [](QProcess::ProcessError err) {
        qWarning() << "AppEngine: hand_detector process error:" << err;
    });

    currentDetectorMode = mode;
    handDetectorProcess->start(python, { scriptPath });
    qDebug() << "AppEngine: hand_detector started, pid =" << handDetectorProcess->processId();

    logger.log(AppEventType::SessionStateChanged,
               {{"hand_detector", "launched"}, {"mode", mode}});
}

void AppEngine::stopHandDetector()
{
    currentDetectorMode.clear();   // prevent auto-restart
    if (!handDetectorProcess) return;
    if (handDetectorProcess->state() != QProcess::NotRunning) {
        handDetectorProcess->terminate();
        handDetectorProcess->waitForFinished(2000);
    }
    handDetectorProcess->deleteLater();
    handDetectorProcess = nullptr;
    if (handDetectorIsRunning) {
        handDetectorIsRunning = false;
        emit handDetectorRunningChanged();
    }

    // Clear gesture state — the UDP feed stops when the process dies, so
    // GlobalState would retain the last packet's values forever otherwise.
    globalState->rightHandVisible.store(false);
    globalState->leftHandVisible.store(false);
    globalState->leftDrumHit.store(false);
    globalState->rightDrumHit.store(false);
}

// ---------------------------------------------------------------------------
// Video frame pipe — Qt owns the camera; frames are forwarded to Python stdin
// ---------------------------------------------------------------------------
void AppEngine::connectVideoSink(QVideoSink* sink)
{
    if (m_videoSink)
        disconnect(m_videoSink, &QVideoSink::videoFrameChanged,
                   this, &AppEngine::onVideoFrame);
    m_videoSink = sink;
    if (m_videoSink) {
        connect(m_videoSink, &QVideoSink::videoFrameChanged,
                this, &AppEngine::onVideoFrame);
        // Kick the pull protocol in case Python is blocked waiting for the first frame.
        QTimer::singleShot(200, this, &AppEngine::pipeNextFrame);
    }
}

void AppEngine::onVideoFrame(const QVideoFrame& frame)
{
    // Convert and scale here (fires on every camera frame, independent of Python).
    // pipeNextFrame() then just does a cheap write with no conversion work.
    QImage img = frame.toImage().convertToFormat(QImage::Format_RGB888);
    if (img.isNull()) return;
    if (img.width() > 128)
        img = img.scaledToWidth(128, Qt::FastTransformation);

    const quint32 w = static_cast<quint32>(img.width());
    const quint32 h = static_cast<quint32>(img.height());
    const int     n = static_cast<int>(w * h * 3);

    QByteArray bytes(n, Qt::Uninitialized);
    for (int y = 0; y < static_cast<int>(h); ++y)
        memcpy(bytes.data() + y * static_cast<int>(w) * 3, img.scanLine(y), w * 3);

    QMutexLocker lock(&m_frameMutex);
    m_frameW     = w;
    m_frameH     = h;
    m_frameBytes = std::move(bytes);
}

void AppEngine::pipeNextFrame()
{
    if (!handDetectorProcess || handDetectorProcess->state() != QProcess::Running)
        return;

    QMutexLocker lock(&m_frameMutex);
    if (m_frameBytes.isEmpty()) return;

    const quint32 hdr[2] = { m_frameW, m_frameH };
    handDetectorProcess->write(reinterpret_cast<const char*>(hdr), 8);
    handDetectorProcess->write(m_frameBytes);
}

} // namespace airchestra
