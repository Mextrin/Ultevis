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
    Q_PROPERTY(QString thereminWaveform    READ thereminWaveform    WRITE setThereminWaveform    NOTIFY thereminWaveformChanged)
    Q_PROPERTY(float   thereminFreqMin     READ thereminFreqMin     WRITE setThereminFreqMin     NOTIFY thereminFreqMinChanged)
    Q_PROPERTY(float   thereminFreqMax     READ thereminFreqMax     WRITE setThereminFreqMax     NOTIFY thereminFreqMaxChanged)
    Q_PROPERTY(float   thereminVolumeFloor READ thereminVolumeFloor WRITE setThereminVolumeFloor NOTIFY thereminVolumeFloorChanged)

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

    float thereminFreqMin() const { return tFreqMin; }
    void setThereminFreqMin(float v) {
        if (!qFuzzyCompare(tFreqMin, v)) { tFreqMin = v; emit thereminFreqMinChanged(); }
    }

    float thereminFreqMax() const { return tFreqMax; }
    void setThereminFreqMax(float v) {
        if (!qFuzzyCompare(tFreqMax, v)) { tFreqMax = v; emit thereminFreqMaxChanged(); }
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
    void thereminFreqMinChanged();
    void thereminFreqMaxChanged();
    void thereminVolumeFloorChanged();

private:
    AppScreen screen    = AppScreen::Landing;
    bool      running   = false;
    bool      midi      = false;
    QString   status    = "Ready";
    float     volume    = 1.0f;
    QString   tWaveform = "sine";
    float     tFreqMin  = 220.0f;
    float     tFreqMax  = 880.0f;
    float     tVolumeFloor = 0.2f;
};

} // namespace airchestra
