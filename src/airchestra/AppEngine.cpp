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
    if (current == AppScreen::Theremin) {
        stopHandDetector();
        globalState->currentInstrument.store(ActiveInstrument::Theremin);
        state.setCurrentScreen(static_cast<int>(AppScreen::Session));
        logger.log(AppEventType::ScreenChanged, {{"screen", "session"}});
        return;
    }
    logger.log(AppEventType::ScreenChanged, {{"screen", "landing"}});
    state.setCurrentScreen(static_cast<int>(AppScreen::Landing));
    state.setSessionRunning(false);
}

void AppEngine::selectInstrument(const QString& name)
{
    logger.log(AppEventType::ButtonClicked, {{"button", "select_instrument"}, {"instrument", name}});

    if (name == QStringLiteral("theremin")) {
        globalState->currentInstrument.store(ActiveInstrument::Theremin);
        launchHandDetector("theremin");
        state.setCurrentScreen(static_cast<int>(AppScreen::Theremin));
        logger.log(AppEventType::ScreenChanged, {{"screen", "theremin"}});
    }
    // Drums / keyboard left for future integration — screens not wired yet
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
                       {{"midi_device", displayName.toStdString()}});
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
    stopHandDetector(); // kill any existing instance first

    // Resolve script path: prefer next to the app bundle, fall back to source tree
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + "/hand_detector.py",
        appDir + "/../Resources/hand_detector.py",   // inside macOS bundle
        QDir::currentPath() + "/src/mediapipe/hand_detector.py",
    };

    QString scriptPath;
    for (const auto& c : candidates) {
        if (QFileInfo::exists(c)) { scriptPath = c; break; }
    }
    if (scriptPath.isEmpty()) {
        qWarning("AppEngine: hand_detector.py not found — gesture input disabled");
        return;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("ULTEVIS_CAMERA_MODE", mode);

    handDetectorProcess = new QProcess(this);
    handDetectorProcess->setProcessEnvironment(env);
    handDetectorProcess->start("python3", { scriptPath });

    logger.log(AppEventType::SessionStateChanged,
               {{"hand_detector", "launched"}, {"mode", mode.toStdString()}});
}

void AppEngine::stopHandDetector()
{
    if (!handDetectorProcess) return;
    if (handDetectorProcess->state() != QProcess::NotRunning) {
        handDetectorProcess->terminate();
        handDetectorProcess->waitForFinished(2000);
    }
    handDetectorProcess->deleteLater();
    handDetectorProcess = nullptr;
}

} // namespace airchestra
