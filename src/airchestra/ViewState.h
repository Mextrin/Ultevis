#pragma once

#include <QObject>
#include <QString>

namespace airchestra
{

enum class AppScreen
{
    Landing,
    Session,
    Theremin,
    Settings,
    About,
    Drums,
    Keyboard,
    Guitar
};

class ViewState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int     currentScreen  READ currentScreen  WRITE setCurrentScreen  NOTIFY currentScreenChanged)
    Q_PROPERTY(bool    sessionRunning READ sessionRunning                          NOTIFY sessionRunningChanged)
    Q_PROPERTY(QString appStatus      READ appStatus                               NOTIFY appStatusChanged)
    Q_PROPERTY(bool    midiEnabled    READ midiEnabled    WRITE setMidiEnabled    NOTIFY midiEnabledChanged)

    // Global audio
    Q_PROPERTY(float   masterVolume   READ masterVolume   WRITE setMasterVolume   NOTIFY masterVolumeChanged)

    // Theremin synthesis settings (persisted across navigation)
    Q_PROPERTY(QString thereminWaveform      READ thereminWaveform      WRITE setThereminWaveform      NOTIFY thereminWaveformChanged)
    Q_PROPERTY(int     thereminSemitoneRange READ thereminSemitoneRange WRITE setThereminSemitoneRange NOTIFY thereminSemitoneRangeChanged)
    Q_PROPERTY(int     thereminCenterNote    READ thereminCenterNote    WRITE setThereminCenterNote    NOTIFY thereminCenterNoteChanged)
    Q_PROPERTY(float   thereminVolumeFloor   READ thereminVolumeFloor   WRITE setThereminVolumeFloor   NOTIFY thereminVolumeFloorChanged)

public:
    explicit ViewState(QObject* parent = nullptr) : QObject(parent) {}

    int currentScreen() const { return static_cast<int>(screen); }
    void setCurrentScreen(int s) {
        auto val = static_cast<AppScreen>(s);
        if (screen != val) { screen = val; emit currentScreenChanged(); }
    }

    bool sessionRunning() const { return running; }
    void setSessionRunning(bool v) {
        if (running != v) { running = v; emit sessionRunningChanged(); }
    }

    QString appStatus() const { return status; }
    void setAppStatus(const QString& s) {
        if (status != s) { status = s; emit appStatusChanged(); }
    }

    bool midiEnabled() const { return midi; }
    void setMidiEnabled(bool v) {
        if (midi != v) { midi = v; emit midiEnabledChanged(); }
    }

    float masterVolume() const { return volume; }
    void setMasterVolume(float v) {
        if (!qFuzzyCompare(volume, v)) { volume = v; emit masterVolumeChanged(); }
    }

    QString thereminWaveform() const { return tWaveform; }
    void setThereminWaveform(const QString& w) {
        if (tWaveform != w) { tWaveform = w; emit thereminWaveformChanged(); }
    }

    int thereminSemitoneRange() const { return tSemitoneRange; }
    void setThereminSemitoneRange(int v) {
        if (tSemitoneRange != v) { tSemitoneRange = v; emit thereminSemitoneRangeChanged(); }
    }

    int thereminCenterNote() const { return tCenterNote; }
    void setThereminCenterNote(int v) {
        if (tCenterNote != v) { tCenterNote = v; emit thereminCenterNoteChanged(); }
    }

    float thereminVolumeFloor() const { return tVolumeFloor; }
    void setThereminVolumeFloor(float v) {
        if (!qFuzzyCompare(tVolumeFloor, v)) { tVolumeFloor = v; emit thereminVolumeFloorChanged(); }
    }

signals:
    void currentScreenChanged();
    void sessionRunningChanged();
    void appStatusChanged();
    void midiEnabledChanged();
    void masterVolumeChanged();
    void thereminWaveformChanged();
    void thereminSemitoneRangeChanged();
    void thereminCenterNoteChanged();
    void thereminVolumeFloorChanged();

private:
    AppScreen screen    = AppScreen::Landing;
    bool      running   = false;
    bool      midi      = false;
    QString   status    = "Ready";
    float     volume    = 1.0f;
    QString   tWaveform      = "sine";
    int       tSemitoneRange = 48;
    int       tCenterNote    = 60;
    float     tVolumeFloor   = 0.00f;
};

} // namespace airchestra
