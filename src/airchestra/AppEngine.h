#pragma once

#include "EventLogger.h"
#include "ViewState.h"

#include <QObject>
#include <QStringList>
#include <QProcess>
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
    Q_PROPERTY(airchestra::ViewState* viewState       READ viewState       CONSTANT)
    Q_PROPERTY(QString                cameraPermission READ cameraPermission NOTIFY cameraPermissionChanged)
    Q_PROPERTY(QStringList            midiDevices     READ midiDevices     CONSTANT)

public:
    explicit AppEngine(QObject* parent = nullptr);
    ~AppEngine(); // must be defined in .cpp where JUCE types are complete

    ViewState*  viewState()        { return &state; }
    QString     cameraPermission() const { return cameraPermissionStatus; }
    QStringList midiDevices()      const { return midiDeviceNames; }

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
    Q_INVOKABLE void setThereminWaveform(const QString& wave); // "sine"|"square"|"sawtooth"|"triangle"
    Q_INVOKABLE void setThereminFreqMin(float hz);
    Q_INVOKABLE void setThereminFreqMax(float hz);
    Q_INVOKABLE void setThereminVolumeFloor(float v);

signals:
    void cameraPermissionChanged();

private:
    void setCameraPermissionStatus(const QString& value);
    void startCameraThread();
    void launchHandDetector(const QString& mode); // "theremin" | "drums"
    void stopHandDetector();

    EventLogger logger;
    ViewState   state;
    QString     cameraPermissionStatus = "undetermined";

    // "None" + system MIDI device names; indices parallel midiDeviceIds
    QStringList              midiDeviceNames;
    std::vector<std::string> midiDeviceIds;

    std::unique_ptr<GlobalState>         globalState;
    std::unique_ptr<HeadlessAudioEngine> audioEngine;

    std::thread       cameraThread;
    std::atomic<bool> cameraStopFlag { false };

    QProcess* handDetectorProcess = nullptr;
};

} // namespace airchestra
