#include "AppEngine.h"

#include <QCoreApplication>
#include <QPermissions>

namespace airchestra {

AppEngine::AppEngine(QObject *parent) : QObject(parent) {
  logger.log(AppEventType::AppStarted, {{"mode", "interactive"}});

  // Seed cameraPermissionStatus from the current Qt-reported state.
  QCameraPermission camPerm;
  switch (qApp->checkPermission(camPerm)) {
  case Qt::PermissionStatus::Granted:
    cameraPermissionStatus = "granted";
    break;
  case Qt::PermissionStatus::Denied:
    cameraPermissionStatus = "denied";
    break;
  case Qt::PermissionStatus::Undetermined:
    cameraPermissionStatus = "undetermined";
    break;
  }
}

void AppEngine::setCameraPermissionStatus(const QString &value) {
  if (cameraPermissionStatus == value)
    return;
  cameraPermissionStatus = value;
  emit cameraPermissionChanged();
}

void AppEngine::requestCameraPermission() {
  QCameraPermission camPerm;
  const auto status = qApp->checkPermission(camPerm);
  if (status == Qt::PermissionStatus::Granted) {
    setCameraPermissionStatus("granted");
    return;
  }
  if (status == Qt::PermissionStatus::Denied) {
    setCameraPermissionStatus("denied");
    return;
  }

  // Not yet determined — this call triggers the system prompt on macOS.
  qApp->requestPermission(camPerm, this, [this](const QPermission &result) {
    switch (result.status()) {
    case Qt::PermissionStatus::Granted:
      logger.log(AppEventType::SessionStateChanged,
                 {{"camera_permission", "granted"}});
      setCameraPermissionStatus("granted");
      break;
    case Qt::PermissionStatus::Denied:
      logger.log(AppEventType::SessionStateChanged,
                 {{"camera_permission", "denied"}});
      setCameraPermissionStatus("denied");
      break;
    case Qt::PermissionStatus::Undetermined:
      setCameraPermissionStatus("undetermined");
      break;
    }
  });
}

void AppEngine::proceed() {
  logger.log(AppEventType::ButtonClicked,
             {{"button", "proceed"}, {"status", "User clicked to proceed"}});
  state.setCurrentScreen(static_cast<int>(AppScreen::Session));
  state.setSessionRunning(true);
  logger.log(AppEventType::ScreenChanged, {{"screen", "session"}});
}

void AppEngine::goBack() {
  const auto current = static_cast<AppScreen>(state.currentScreen());
  if (current == AppScreen::Theremin) {
    // Popping from Theremin returns to the instrument-select (Session) screen.
    state.setCurrentScreen(static_cast<int>(AppScreen::Session));
    logger.log(AppEventType::ScreenChanged, {{"screen", "session"}});
    return;
  }

  logger.log(AppEventType::ScreenChanged, {{"screen", "landing"}});
  state.setCurrentScreen(static_cast<int>(AppScreen::Landing));
  state.setSessionRunning(false);
}

void AppEngine::setMidiEnabled(bool enabled) {
  state.setMidiEnabled(enabled);
  logger.log(AppEventType::SessionStateChanged,
             {{"midi_output", enabled ? "on" : "off"}});
}

void AppEngine::selectInstrument(const QString &name) {
  logger.log(AppEventType::ButtonClicked,
             {{"button", "select_instrument"}, {"instrument", name}});
  if (name == QStringLiteral("theremin")) {
    state.setCurrentScreen(static_cast<int>(AppScreen::Theremin));
    logger.log(AppEventType::ScreenChanged, {{"screen", "theremin"}});
  }
}

} // namespace airchestra
