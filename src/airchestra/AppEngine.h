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

public:
    explicit AppEngine(QObject* parent = nullptr);

    ViewState* viewState() { return &state; }

    Q_INVOKABLE void proceed();
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void setMidiEnabled(bool enabled);

private:
    EventLogger logger;
    ViewState state;
};

}
