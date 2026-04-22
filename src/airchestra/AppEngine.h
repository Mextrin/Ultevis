#pragma once

#include <QObject>
#include <QString>
#include "EventLogger.h"
#include "ViewState.h"

// Backend Headers
#include "Core/GlobalState.h"
#include "Audio/AudioEngine.h"

namespace airchestra {

class AppEngine : public QObject {
    Q_OBJECT
    // This exposes the camera permission string to QML
    Q_PROPERTY(QString cameraPermissionStatus READ getCameraPermissionStatus NOTIFY cameraPermissionChanged)

public:
    // Constructor with our new backend pointers
    explicit AppEngine(GlobalState* gState, HeadlessAudioEngine* aEngine, QObject *parent = nullptr);

    // --- The Q_INVOKABLE methods QML is allowed to call ---
    Q_INVOKABLE void requestCameraPermission();
    Q_INVOKABLE void proceed();
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void setMidiEnabled(bool enabled);
    Q_INVOKABLE void selectInstrument(const QString &name);

    // Property getter
    QString getCameraPermissionStatus() const { return cameraPermissionStatus; }

signals:
    // Fired when the permission string updates
    void cameraPermissionChanged();

private:
    // Internal helper function
    void setCameraPermissionStatus(const QString &value);

    // Frontend State
    EventLogger logger;
    ViewState state;
    QString cameraPermissionStatus = "undetermined";

    // Backend Pointers
    GlobalState* globalState;
    HeadlessAudioEngine* audioEngine;
};

} // namespace airchestra