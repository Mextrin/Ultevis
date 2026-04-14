#include "AppEngine.h"

namespace airchestra
{

AppEngine::AppEngine(QObject* parent)
    : QObject(parent)
{
    logger.log(AppEventType::AppStarted, {{"mode", "interactive"}});
}

void AppEngine::proceed()
{
    logger.log(AppEventType::ButtonClicked, {{"button", "proceed"}, {"status", "User clicked to proceed"}});
    state.setCurrentScreen(static_cast<int>(AppScreen::Session));
    state.setSessionRunning(true);
    logger.log(AppEventType::ScreenChanged, {{"screen", "session"}});
}

void AppEngine::goBack()
{
    logger.log(AppEventType::ScreenChanged, {{"screen", "landing"}});
    state.setCurrentScreen(static_cast<int>(AppScreen::Landing));
    state.setSessionRunning(false);
}

void AppEngine::setMidiEnabled(bool enabled)
{
    state.setMidiEnabled(enabled);
    logger.log(AppEventType::SessionStateChanged, {{"midi_output", enabled ? "on" : "off"}});
}

}
