#pragma once

#include "EventLogger.h"
#include "ViewState.h"

#include <QObject>
#include <QStringList>
#include <QProcess>
#include <QImage>
#include <QVideoSink>
#include <QVideoFrame>
#include <QByteArray>
#include <QMutex>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include <string>

// Forward-declared so JUCE headers stay out of the global include tree.
class GlobalState;
class HeadlessAudioEngine;

namespace airchestra
{

class AppEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(airchestra::ViewState* viewState           READ viewState           CONSTANT)
    Q_PROPERTY(QString                cameraPermission    READ cameraPermission    NOTIFY cameraPermissionChanged)
    Q_PROPERTY(QStringList            midiDevices         READ midiDevices         CONSTANT)
    Q_PROPERTY(bool                   handDetectorRunning READ handDetectorRunning NOTIFY handDetectorRunningChanged)

public:
    explicit AppEngine(QObject* parent = nullptr);
    ~AppEngine(); // must be defined in .cpp where JUCE types are complete

    ViewState*  viewState()           { return &state; }
    QString     cameraPermission()    const { return cameraPermissionStatus; }
    QStringList midiDevices()         const { return midiDeviceNames; }
    bool        handDetectorRunning() const { return handDetectorIsRunning; }

    // Navigation
    Q_INVOKABLE void proceed();
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void selectInstrument(const QString& name);
    Q_INVOKABLE void requestCameraPermission();

    // MIDI output selection (called from InstrumentSelectPage)
    Q_INVOKABLE void selectMidiDevice(const QString& displayName);
    Q_INVOKABLE void setMidiEnabled(bool enabled); // kept for compatibility

    // Theremin settings – called from QML whenever a control changes
    Q_INVOKABLE void setMasterVolume(float v);
    Q_INVOKABLE void setThereminWaveform(const QString& wave);
    Q_INVOKABLE void setThereminFreqMin(float hz);
    Q_INVOKABLE void setThereminFreqMax(float hz);
    Q_INVOKABLE void setThereminVolumeFloor(float v);

    // Drums — called from QML drum pad clicks
    Q_INVOKABLE void triggerDrumHit(int midiNote, int velocity);

    // Keyboard — called from QML piano key press/release
    Q_INVOKABLE void triggerKeyboardNote(int midiNote, int velocity);
    Q_INVOKABLE void releaseKeyboardNote();

    // Called from QML once the camera is active; taps into VideoOutput's sink
    // so we can pipe frames to hand_detector without competing for the camera.
    Q_INVOKABLE void connectVideoSink(QVideoSink* sink);

signals:
    void cameraPermissionChanged();
    void handDetectorRunningChanged();

private:
    void setCameraPermissionStatus(const QString& value);
    void startCameraThread();
    void launchHandDetector(const QString& mode); // "theremin" | "drums"
    void stopHandDetector();
    void onVideoFrame(const QVideoFrame& frame);
    void pipeNextFrame();

    EventLogger logger;
    ViewState   state;
    QString     cameraPermissionStatus = "undetermined";
    bool        handDetectorIsRunning  = false;

    QStringList              midiDeviceNames;
    std::vector<std::string> midiDeviceIds;

    std::unique_ptr<GlobalState>         globalState;
    std::unique_ptr<HeadlessAudioEngine> audioEngine;

    std::thread       cameraThread;
    std::atomic<bool> cameraStopFlag { false };

    QString      currentDetectorMode;
    QProcess*    handDetectorProcess = nullptr;
    QVideoSink*  m_videoSink         = nullptr;

    // Pre-converted frame bytes (128px wide RGB888) ready to write to Python stdin.
    // Updated on every camera frame; written only when Python sends 'R'.
    QMutex     m_frameMutex;
    QByteArray m_frameBytes;
    quint32    m_frameW = 0;
    quint32    m_frameH = 0;
};

} // namespace airchestra
