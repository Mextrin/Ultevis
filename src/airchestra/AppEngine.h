#pragma once
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
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
    Q_PROPERTY(QString currentMidiDevice READ getCurrentMidiDevice NOTIFY currentMidiDeviceChanged)

    // Hand-tracking state mirrored from GlobalState so QML can bind to it.
    Q_PROPERTY(bool leftHandVisible READ leftHandVisible WRITE setLeftHandVisible NOTIFY handStateChanged)
    Q_PROPERTY(bool rightHandVisible READ rightHandVisible WRITE setRightHandVisible NOTIFY handStateChanged)
    Q_PROPERTY(qreal leftHandX READ leftHandX  NOTIFY handStateChanged)
    Q_PROPERTY(qreal leftHandY READ leftHandY WRITE setLeftHandY NOTIFY handStateChanged)
    Q_PROPERTY(qreal rightHandX READ rightHandX WRITE setRightHandX NOTIFY handStateChanged)
    Q_PROPERTY(qreal rightHandY READ rightHandY NOTIFY handStateChanged)
    Q_PROPERTY(bool leftPinch READ leftPinch NOTIFY handStateChanged)
    Q_PROPERTY(bool rightPinch READ rightPinch NOTIFY handStateChanged)
    Q_PROPERTY(bool leftThumbUp READ leftThumbUp NOTIFY handStateChanged)
    Q_PROPERTY(bool leftThumbDown READ leftThumbDown NOTIFY handStateChanged)
    Q_PROPERTY(bool rightThumbUp READ rightThumbUp NOTIFY handStateChanged)
    Q_PROPERTY(bool rightThumbDown  READ rightThumbDown  NOTIFY handStateChanged)
    Q_PROPERTY(bool guitarNeckUp    READ guitarNeckUp    NOTIFY handStateChanged)
    Q_PROPERTY(bool guitarNeckDown  READ guitarNeckDown  NOTIFY handStateChanged)
    Q_PROPERTY(bool guitarStrumHit  READ guitarStrumHit  NOTIFY handStateChanged)

    // Keyboard octave state (read/write via slots).
    Q_PROPERTY(int topKeyboardOctave READ topKeyboardOctave NOTIFY keyboardOctavesChanged)
    Q_PROPERTY(int bottomKeyboardOctave READ bottomKeyboardOctave NOTIFY keyboardOctavesChanged)
    Q_PROPERTY(QVariantList activeKeyboardNotes READ activeKeyboardNotes NOTIFY activeKeyboardNotesChanged)
    Q_PROPERTY(QVariantList activeTopKeyboardNotes READ activeTopKeyboardNotes NOTIFY activeKeyboardNotesChanged)
    Q_PROPERTY(QVariantList activeBottomKeyboardNotes READ activeBottomKeyboardNotes NOTIFY activeKeyboardNotesChanged)

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
    QString getCurrentMidiDevice() const { return m_currentMidiDevice; }

    Q_INVOKABLE void setMasterVolume(float v);
    Q_INVOKABLE void setLeftDrumVelocity(int v);
    Q_INVOKABLE void setRightDrumVelocity(int v);
    Q_INVOKABLE void setLeftKeyboardVelocity(int v);
    Q_INVOKABLE void setRightKeyboardVelocity(int v);
    Q_INVOKABLE void setThereminWaveform(const QString& wave);
    Q_INVOKABLE void setThereminSemitoneRangeOneSide(int semitones);
    Q_INVOKABLE void setThereminCenterNote(int midiNote);
    Q_INVOKABLE void setThereminVolumeFloor(float v);

    Q_INVOKABLE void triggerDrumHit(int midiNote, int velocity);
    Q_INVOKABLE void setMouthKickEnabled(bool enabled);
    Q_INVOKABLE void triggerKeyboardNote(int midiNote, int velocity);
    Q_INVOKABLE void releaseKeyboardNote(int midiNote);
    Q_INVOKABLE void adjustKeyboardOctave(int keyboardIndex, int delta);
    Q_INVOKABLE void setSustainPedal(bool enabled);
    Q_INVOKABLE void setKeyboardInstrument(int instrumentID);

    Q_INVOKABLE void setGuitarSound(int soundID);
    Q_INVOKABLE void triggerGuitarStrum(int velocity);

    bool guitarNeckUp()   const { return m_guitarNeckUp;   }
    bool guitarNeckDown() const { return m_guitarNeckDown; }
    bool guitarStrumHit() const { return m_guitarStrumHit; }

    // --- ADDED SETTERS for theremin jingle---
    Q_INVOKABLE void setRightHandVisible(bool visible);
    Q_INVOKABLE void setLeftHandVisible(bool visible);
    Q_INVOKABLE void setRightHandX(qreal x);
    Q_INVOKABLE void setLeftHandY(qreal y);
    // ---------------------

    bool leftHandVisible() const { return m_leftHandVisible; }
    bool rightHandVisible() const { return m_rightHandVisible; }
    qreal leftHandX() const { return m_leftHandX; }
    qreal leftHandY() const { return m_leftHandY; }
    qreal rightHandX() const { return m_rightHandX; }
    qreal rightHandY() const { return m_rightHandY; }
    bool leftPinch() const { return m_leftPinch; }
    bool rightPinch() const { return m_rightPinch; }
    bool leftThumbUp() const { return m_leftThumbUp; }
    bool leftThumbDown() const { return m_leftThumbDown; }
    bool rightThumbUp() const { return m_rightThumbUp; }
    bool rightThumbDown() const { return m_rightThumbDown; }
    int topKeyboardOctave() const { return m_topKeyboardOctave; }
    int bottomKeyboardOctave() const { return m_bottomKeyboardOctave; }
    QVariantList activeKeyboardNotes() const { return m_activeKeyboardNotes; }
    QVariantList activeTopKeyboardNotes() const { return m_activeTopKeyboardNotes; }
    QVariantList activeBottomKeyboardNotes() const { return m_activeBottomKeyboardNotes; }

    std::vector<std::pair<std::string, std::string>> getAvailableMidiDevices();

signals:
    void cameraPermissionChanged();
    void currentMidiDeviceChanged();
    void handStateChanged();
    void keyboardOctavesChanged();
    void activeKeyboardNotesChanged();

private:
    void setCameraPermissionStatus(const QString &value);
    void refreshTrackedState();
    void scheduleCameraModeStart(const QString& mode);
    void launchPendingCameraMode();

    QString cameraPermissionStatus{"undetermined"};
    ViewState state;
    EventLogger logger;

    GlobalState* globalState;
    HeadlessAudioEngine* audioEngine;

    QStringList midiDeviceNames;
    std::vector<std::string> midiDeviceIds;
    QString m_currentMidiDevice = "None";
    QTimer cameraModeDelayTimer;
    QString pendingCameraMode = "none";
    QTimer handStatePollTimer;

    bool m_leftHandVisible = false;
    bool m_rightHandVisible = false;
    qreal m_leftHandX = 0.5;
    qreal m_leftHandY = 1.0;
    qreal m_rightHandX = 0.5;
    qreal m_rightHandY = 0.5;
    bool m_leftPinch = false;
    bool m_rightPinch = false;
    bool m_leftThumbUp    = false;
    bool m_leftThumbDown  = false;
    bool m_rightThumbUp   = false;
    bool m_rightThumbDown = false;
    bool m_guitarNeckUp   = false;
    bool m_guitarNeckDown = false;
    bool m_guitarStrumHit = false;
    int m_topKeyboardOctave = 3;
    int m_bottomKeyboardOctave = 5;
    QVariantList m_activeKeyboardNotes;
    QVariantList m_activeTopKeyboardNotes;
    QVariantList m_activeBottomKeyboardNotes;
};

} // namespace airchestra
