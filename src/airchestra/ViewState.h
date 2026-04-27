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
    Drums,
    Keyboard,
    Settings,
    About
};

class ViewState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentScreen READ currentScreen WRITE setCurrentScreen NOTIFY currentScreenChanged)
    Q_PROPERTY(bool sessionRunning READ sessionRunning NOTIFY sessionRunningChanged)
    Q_PROPERTY(QString appStatus READ appStatus NOTIFY appStatusChanged)
    Q_PROPERTY(bool midiEnabled READ midiEnabled WRITE setMidiEnabled NOTIFY midiEnabledChanged)

    Q_PROPERTY(int thereminCenterNote READ thereminCenterNote WRITE setThereminCenterNote NOTIFY thereminCenterNoteChanged)
    Q_PROPERTY(int thereminSemitoneRange READ thereminSemitoneRange WRITE setThereminSemitoneRange NOTIFY thereminSemitoneRangeChanged)
    Q_PROPERTY(float thereminVolumeFloor READ thereminVolumeFloor WRITE setThereminVolumeFloor NOTIFY thereminVolumeFloorChanged)

public:
    explicit ViewState(QObject* parent = nullptr) : QObject(parent) {}

    int currentScreen() const { return static_cast<int>(screen); }
    void setCurrentScreen(int s)
    {
        auto val = static_cast<AppScreen>(s);
        if (screen != val) { screen = val; emit currentScreenChanged(); }
    }

    bool sessionRunning() const { return running; }
    void setSessionRunning(bool v)
    {
        if (running != v) { running = v; emit sessionRunningChanged(); }
    }

    QString appStatus() const { return status; }
    void setAppStatus(const QString& s)
    {
        if (status != s) { status = s; emit appStatusChanged(); }
    }

    bool midiEnabled() const { return midi; }
    void setMidiEnabled(bool v)
    {
        if (midi != v) { midi = v; emit midiEnabledChanged(); }
    }

    // --- GETTERS & SETTERS FOR THEREMIN ---
    int thereminCenterNote() const { return m_thereminCenterNote; }
    void setThereminCenterNote(int v)
    {
        if (m_thereminCenterNote != v) { m_thereminCenterNote = v; emit thereminCenterNoteChanged(); }
    }

    int thereminSemitoneRange() const { return m_thereminSemitoneRange; }
    void setThereminSemitoneRange(int v)
    {
        if (m_thereminSemitoneRange != v) { m_thereminSemitoneRange = v; emit thereminSemitoneRangeChanged(); }
    }

    float thereminVolumeFloor() const { return m_thereminVolumeFloor; }
    void setThereminVolumeFloor(float v)
    {
        if (m_thereminVolumeFloor != v) { m_thereminVolumeFloor = v; emit thereminVolumeFloorChanged(); }
    }

signals:
    void currentScreenChanged();
    void sessionRunningChanged();
    void appStatusChanged();
    void midiEnabledChanged();

    void thereminCenterNoteChanged();
    void thereminSemitoneRangeChanged();
    void thereminVolumeFloorChanged();

private:
    AppScreen screen = AppScreen::Landing;
    bool running = false;
    bool midi = false;
    QString status = "Ready";

    // --- DEFAULT THEREMIN VALUES ---
    int m_thereminCenterNote = 60;       // Middle C
    int m_thereminSemitoneRange = 24;    // 2 Octaves
    float m_thereminVolumeFloor = 0.0f;  // 0% Volume Floor
};

}
