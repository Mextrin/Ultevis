#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <thread>
#include <cstdlib>
#include <string>
#include <QProcess>

// Frontend Headers
#include "airchestra/AppEngine.h"
#include "airchestra/VideoReceiver.h"

// Backend Headers
#include "Core/GlobalState.h"
#include "Audio/AudioEngine.h"
#include "juce_events/juce_events.h"

extern void startCameraFeed(GlobalState* state);

QProcess* pythonProcess = nullptr;

void launchHandDetectorIfRequested(const GlobalState& state)
{
    if (std::getenv("ULTEVIS_LAUNCH_HAND_DETECTOR") == nullptr)
        return;

    const char* detectorScript = std::getenv("ULTEVIS_HAND_DETECTOR_SCRIPT");
    const QString scriptPath = detectorScript != nullptr
        ? QString(detectorScript)
        : "src/mediapipe/hand_detector.py"; 

    const QString cameraMode = state.currentInstrument.load() == ActiveInstrument::Drums
        ? "drums"
        : "theremin";

    if (!pythonProcess) {
        pythonProcess = new QProcess(qApp);
    }

    // Set the environment variable so Python knows which instrument to load
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("ULTEVIS_CAMERA_MODE", cameraMode);
    
    // Optional: Hide the Python output spam
    env.insert("GLOG_minloglevel", "2");
    env.insert("TF_CPP_MIN_LOG_LEVEL", "3");
    
    pythonProcess->setProcessEnvironment(env);

    pythonProcess->setProcessChannelMode(QProcess::ForwardedChannels);
    
    // Launch Python natively
    pythonProcess->start("python", QStringList() << scriptPath);
}

int main(int argc, char* argv[])
{
    //BOOT QT
    QGuiApplication app(argc, argv);
    app.setApplicationName("Airchestra");
    app.setApplicationVersion("0.2.0");
    app.setWindowIcon(QIcon(":/assets/icons/app-icon.png"));
    QQuickStyle::setStyle("Basic");

    //BOOT JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;

    //BOOT AUDIO + CAMERA BACKEND
    GlobalState globalState;
    AudioEngineConfig audioConfig;
    HeadlessAudioEngine audioEngine(&globalState, audioConfig);

    //START PYTHON AND UDP LISTENER
    launchHandDetectorIfRequested(globalState);
    std::thread udpThread([&globalState]() {
        startCameraFeed(&globalState);
    });
    udpThread.detach();

    //BOOT UI ENGINE
    airchestra::AppEngine engine(&globalState, &audioEngine);

    QQmlApplicationEngine qmlEngine;
    qmlEngine.rootContext()->setContextProperty("appEngine", &engine);

    auto* cameraProvider = new airchestra::CameraImageProvider();
    qmlEngine.addImageProvider("camera", cameraProvider);
    airchestra::VideoReceiver receiver(cameraProvider);
    // -----------------------------------------

    qmlEngine.load(QUrl("qrc:/qml/main.qml"));

    if (qmlEngine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}