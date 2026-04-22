#include "AppEngine.h"
#include <QCoreApplication>

// --- Network headers for sendCommandToPython ---
#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

namespace {
    // Helper to switch the Python camera UI instantly
    void sendCommandToPython(const std::string& mode, bool quit = false) 
    {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(5006); 
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        std::string json = "{\"mode\": \"" + mode + "\", \"quit\": " + (quit ? "true" : "false") + "}";
        sendto(sock, json.c_str(), json.length(), 0, (sockaddr*)&addr, sizeof(addr));

        #ifdef _WIN32
            closesocket(sock);
        #else
            close(sock);
        #endif
    }
}

namespace airchestra {

// Constructor saves the pointers and boots permissions
AppEngine::AppEngine(GlobalState* gState, HeadlessAudioEngine* aEngine, QObject *parent) 
    : QObject(parent), globalState(gState), audioEngine(aEngine) 
{
    logger.log(AppEventType::AppStarted, {{"mode", "interactive"}});

    // --- THE FIX ---
    // Since Python actually handles the hardware camera, we bypass Qt's strict 
    // macOS Info.plist permission checks completely!
    cameraPermissionStatus = "granted";
}

void AppEngine::setCameraPermissionStatus(const QString &value) {
    if (cameraPermissionStatus == value)
        return;
    cameraPermissionStatus = value;
    emit cameraPermissionChanged();
}

void AppEngine::requestCameraPermission() {
    // Automatically grant it so QML hides the "Waiting for Camera" overlay
    setCameraPermissionStatus("granted");
}

void AppEngine::proceed() {
    logger.log(AppEventType::ButtonClicked, {{"button", "proceed"}, {"status", "User clicked to proceed"}});
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
    
    // Update the C++ audio engine state
    globalState->routeToMidiOut.store(enabled);
    
    logger.log(AppEventType::SessionStateChanged, {{"midi_output", enabled ? "on" : "off"}});
}

void AppEngine::selectInstrument(const QString &name) {
    logger.log(AppEventType::ButtonClicked, {{"button", "select_instrument"}, {"instrument", name}});
    
    if (name == QStringLiteral("theremin")) {
        state.setCurrentScreen(static_cast<int>(AppScreen::Theremin));
        globalState->currentInstrument.store(ActiveInstrument::Theremin);
        sendCommandToPython("theremin");
        logger.log(AppEventType::ScreenChanged, {{"screen", "theremin"}});
    }
    else if (name == QStringLiteral("drums")) {
        state.setCurrentScreen(static_cast<int>(AppScreen::Drums)); 
        globalState->currentInstrument.store(ActiveInstrument::Drums);
        audioEngine->loadDrumSound("Instruments/SMDrums_Sforzando_1.2/Programs/SM_Drums_kit.sfz");
        sendCommandToPython("drums");
        logger.log(AppEventType::ScreenChanged, {{"screen", "drums"}});
    }
}

} // namespace airchestra