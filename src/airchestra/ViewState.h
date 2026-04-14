#pragma once

#include <QObject>
#include <QString>

namespace airchestra
{

enum class AppScreen
{
    Landing,
    Session,
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

signals:
    void currentScreenChanged();
    void sessionRunningChanged();
    void appStatusChanged();
    void midiEnabledChanged();

private:
    AppScreen screen = AppScreen::Landing;
    bool running = false;
    bool midi = false;
    QString status = "Ready";
};

}
