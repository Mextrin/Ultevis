#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QFileInfo>
#include <QDir>
#include <thread>
#include <cstdlib>
#include <iostream>
#include <string>
#include <QProcess>

// Frontend Headers
#include "airchestra/AppEngine.h"
#include "airchestra/RuntimePaths.h"
#include "airchestra/VideoReceiver.h"

// Backend Headers
#include "Core/GlobalState.h"
#include "Audio/AudioEngine.h"
#include "juce_events/juce_events.h"

extern void startCameraFeed(GlobalState* state);

QProcess* pythonProcess = nullptr;

bool startPythonProcess(const QString& program, const QStringList& arguments)
{
    pythonProcess->start(program, arguments);
    if (pythonProcess->waitForStarted(3000))
        return true;

    std::cerr << "Failed to start " << program.toStdString()
              << ": " << pythonProcess->errorString().toStdString() << std::endl;
    return false;
}

void launchHandDetector(const GlobalState& state)
{
    const char* launchFlag = std::getenv("ULTEVIS_LAUNCH_HAND_DETECTOR");
    if (launchFlag != nullptr) {
        const QString value = QString::fromLocal8Bit(launchFlag).trimmed().toLower();
        if (value == "0" || value == "false" || value == "off" || value == "no")
            return;
    }

    const char* detectorScript = std::getenv("ULTEVIS_HAND_DETECTOR_SCRIPT");
    QString binaryName = "mediapipe/hand_detector";
#ifdef _WIN32
    binaryName += ".exe";
#endif
    const QString scriptPath = detectorScript != nullptr
        ? QString(detectorScript)
        : airchestra::runtimePath(binaryName);

    QFileInfo scriptInfo(scriptPath);
    if (!scriptInfo.exists()) {
        std::cerr << "ERROR: Hand detector script not found: "
                  << QDir::toNativeSeparators(scriptPath).toStdString() << std::endl;
        return;
    }

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

#ifdef _WIN32
    pythonProcess->setProcessChannelMode(QProcess::SeparateChannels);
#else
    pythonProcess->setProcessChannelMode(QProcess::ForwardedChannels);
#endif

    pythonProcess->setWorkingDirectory(scriptInfo.absolutePath());

    if (startPythonProcess(scriptPath, QStringList())) {
        std::cout << "Successfully launched standalone hand_detector binary!" << std::endl;
        return;
    }
    
    std::cerr << "Failed to launch the standalone binary." << std::endl;
}

int main(int argc, char* argv[])
{
    //BOOT QT
    QGuiApplication app(argc, argv);
    app.setApplicationName("Airchestra");
    app.setApplicationVersion("0.2.0");
    app.setWindowIcon(QIcon(":/assets/icons/app-icon.png"));
    QQuickStyle::setStyle("Basic");
    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        airchestra::writeBlackCameraFrame();

        if (pythonProcess) {
            std::cout << "[Airchestra] Shutting down Camera Engine..." << std::endl;
    #ifdef _WIN32
            qint64 pid = pythonProcess->processId();
            if (pid > 0) {
                QProcess::execute("taskkill", {"/F", "/T", "/PID", QString::number(pid)});
            }
            pythonProcess->waitForFinished(1000);
            pythonProcess->kill();
    #else
            pythonProcess->terminate();
            pythonProcess->waitForFinished(3000);
            pythonProcess->kill();
    #endif
        }
    });

    //BOOT JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;

    //BOOT AUDIO + CAMERA BACKEND
    GlobalState globalState;
    AudioEngineConfig audioConfig;
    HeadlessAudioEngine audioEngine(&globalState, audioConfig);

    //START PYTHON AND UDP LISTENER
    launchHandDetector(globalState);
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
