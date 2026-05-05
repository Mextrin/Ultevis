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
    Q_PROPERTY(bool mouthKickEnabled READ mouthKickEnabled WRITE setMouthKickEnabled NOTIFY mouthKickEnabledChanged)


    Q_PROPERTY(int thereminCenterNote READ thereminCenterNote WRITE setThereminCenterNote NOTIFY thereminCenterNoteChanged)
    Q_PROPERTY(int thereminSemitoneRangeOneSide READ thereminSemitoneRangeOneSide WRITE setThereminSemitoneRangeOneSide NOTIFY thereminSemitoneRangeOneSideChanged)
    Q_PROPERTY(float thereminVolumeFloor READ thereminVolumeFloor WRITE setThereminVolumeFloor NOTIFY thereminVolumeFloorChanged)

    Q_PROPERTY(float masterVolume READ masterVolume WRITE setMasterVolume NOTIFY masterVolumeChanged)

    Q_PROPERTY(int leftDrumVelocity READ leftDrumVelocity WRITE setLeftDrumVelocity NOTIFY leftDrumVelocityChanged)
    Q_PROPERTY(int rightDrumVelocity READ rightDrumVelocity WRITE setRightDrumVelocity NOTIFY rightDrumVelocityChanged)

    Q_PROPERTY(int leftKeyboardVelocity READ leftKeyboardVelocity WRITE setLeftKeyboardVelocity NOTIFY leftKeyboardVelocityChanged)
    Q_PROPERTY(int rightKeyboardVelocity READ rightKeyboardVelocity WRITE setRightKeyboardVelocity NOTIFY rightKeyboardVelocityChanged)

    Q_PROPERTY(bool sustainPedal READ sustainPedal WRITE setSustainPedal NOTIFY sustainPedalChanged)


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

    bool mouthKickEnabled() const { return m_mouthKickEnabled; }
    void setMouthKickEnabled(bool v)
    {
        if (m_mouthKickEnabled != v) { m_mouthKickEnabled = v; emit mouthKickEnabledChanged(); }
    }

    // --- GETTERS & SETTERS FOR THEREMIN ---
    int thereminCenterNote() const { return m_thereminCenterNote; }
    void setThereminCenterNote(int v)
    {
        if (m_thereminCenterNote != v) { m_thereminCenterNote = v; emit thereminCenterNoteChanged(); }
    }

    int thereminSemitoneRangeOneSide() const { return m_thereminSemitoneRangeOneSide; }
    void setThereminSemitoneRangeOneSide(int v)
    {
        if (m_thereminSemitoneRangeOneSide != v) { m_thereminSemitoneRangeOneSide = v; emit thereminSemitoneRangeOneSideChanged(); }
    }

    float thereminVolumeFloor() const { return m_thereminVolumeFloor; }
    void setThereminVolumeFloor(float v)
    {
        if (m_thereminVolumeFloor != v) { m_thereminVolumeFloor = v; emit thereminVolumeFloorChanged(); }
    }

    float masterVolume() const { return m_masterVolume; }
    void setMasterVolume(float v)
    {
        if (m_masterVolume != v) { m_masterVolume = v; emit masterVolumeChanged(); }
    }

    int leftDrumVelocity() const { return m_leftDrumVelocity; }
    void setLeftDrumVelocity(int v)
    {
        if (m_leftDrumVelocity != v) { m_leftDrumVelocity = v; emit leftDrumVelocityChanged(); }
    }

    int rightDrumVelocity() const { return m_rightDrumVelocity; }
    void setRightDrumVelocity(int v)
    {
        if (m_rightDrumVelocity != v) { m_rightDrumVelocity = v; emit rightDrumVelocityChanged(); }
    }

    int leftKeyboardVelocity() const { return m_leftKeyboardVelocity; }
    void setLeftKeyboardVelocity(int v)
    {
        if (m_leftKeyboardVelocity != v) { m_leftKeyboardVelocity = v; emit leftKeyboardVelocityChanged(); }
    }

    int rightKeyboardVelocity() const { return m_rightKeyboardVelocity; }
    void setRightKeyboardVelocity(int v)
    {
        if (m_rightKeyboardVelocity != v) { m_rightKeyboardVelocity = v; emit rightKeyboardVelocityChanged(); }
    }

    bool sustainPedal() const { return m_sustainPedal; }
    void setSustainPedal(bool v)
    {
        if (m_sustainPedal != v) { m_sustainPedal = v; emit sustainPedalChanged(); }
    }

signals:
    void currentScreenChanged();
    void sessionRunningChanged();
    void appStatusChanged();
    void midiEnabledChanged();
    void mouthKickEnabledChanged();

    void thereminCenterNoteChanged();
    void thereminSemitoneRangeOneSideChanged();
    void thereminVolumeFloorChanged();

    void masterVolumeChanged();
    void leftDrumVelocityChanged();
    void rightDrumVelocityChanged();
    void leftKeyboardVelocityChanged();
    void rightKeyboardVelocityChanged();
    void sustainPedalChanged();


private:
    AppScreen screen = AppScreen::Landing;
    bool running = false;
    bool midi = false;
    QString status = "Ready";

    // --- DEFAULT THEREMIN VALUES ---
    int m_thereminCenterNote = 60;       // Middle C
    int m_thereminSemitoneRangeOneSide = 24;    // 2 Octaves per side 
    float m_thereminVolumeFloor = 0.0f;  // 0% Volume Floor

    float m_masterVolume = 1.0f;
    int m_leftDrumVelocity = 100;
    int m_rightDrumVelocity = 100;
    int m_leftKeyboardVelocity = 100;
    int m_rightKeyboardVelocity = 100;
    bool m_mouthKickEnabled = false;
    bool m_sustainPedal = false;

};

}
