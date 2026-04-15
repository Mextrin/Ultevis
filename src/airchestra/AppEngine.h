#pragma once

#include "EventLogger.h"
#include "ViewState.h"

#include <QObject>

namespace airchestra
{

class AppEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(airchestra::ViewState* viewState READ viewState CONSTANT)
    Q_PROPERTY(QString cameraPermission READ cameraPermission NOTIFY cameraPermissionChanged)

public:
    explicit AppEngine(QObject* parent = nullptr);

    ViewState* viewState() { return &state; }

    // "granted" | "denied" | "undetermined"
    QString cameraPermission() const { return cameraPermissionStatus; }

    Q_INVOKABLE void proceed();
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void setMidiEnabled(bool enabled);
    Q_INVOKABLE void selectInstrument(const QString& name);

    // Triggers the macOS/system camera permission prompt if not yet determined.
    Q_INVOKABLE void requestCameraPermission();

signals:
    void cameraPermissionChanged();

private:
    void setCameraPermissionStatus(const QString& value);

    EventLogger logger;
    ViewState state;
    QString cameraPermissionStatus = "undetermined";
};

}
