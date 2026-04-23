#pragma once
#include <QObject>
#include <QStringList>
#include <memory>
#include <vector>
#include <string>

#include "ViewState.h"
#include "EventLogger.h"

class GlobalState;
class HeadlessAudioEngine;

namespace airchestra {

class AppEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString cameraPermissionStatus READ cameraPermission NOTIFY cameraPermissionChanged)
    Q_PROPERTY(ViewState* viewState READ getViewState CONSTANT)
    Q_PROPERTY(QStringList midiDeviceNames READ getMidiDeviceNames CONSTANT)

public:
    AppEngine(GlobalState* gState, HeadlessAudioEngine* aEngine, QObject *parent = nullptr);
    ~AppEngine();

    QString cameraPermission() const { return cameraPermissionStatus; }
    ViewState* getViewState() { return &state; }
    QStringList getMidiDeviceNames() const { return midiDeviceNames; }

    Q_INVOKABLE void requestCameraPermission();
    Q_INVOKABLE void proceed();
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void selectInstrument(const QString &name);
    
    Q_INVOKABLE void setMidiEnabled(bool enabled);
    Q_INVOKABLE void selectMidiDevice(const QString& displayName);

    Q_INVOKABLE void setMasterVolume(float v);
    Q_INVOKABLE void setThereminWaveform(const QString& wave);
    Q_INVOKABLE void setThereminSemitoneRange(int semitones);
    Q_INVOKABLE void setThereminCenterNote(int midiNote);
    Q_INVOKABLE void setThereminVolumeFloor(float v);

    Q_INVOKABLE void triggerDrumHit(int midiNote, int velocity);
    Q_INVOKABLE void triggerKeyboardNote(int midiNote, int velocity);
    Q_INVOKABLE void releaseKeyboardNote();

    std::vector<std::pair<std::string, std::string>> getAvailableMidiDevices();

signals:
    void cameraPermissionChanged();

private:
    void setCameraPermissionStatus(const QString &value);

    QString cameraPermissionStatus{"undetermined"};
    ViewState state;
    EventLogger logger;

    GlobalState* globalState;
    HeadlessAudioEngine* audioEngine;

    QStringList midiDeviceNames;
    std::vector<std::string> midiDeviceIds;
};

} // namespace airchestra