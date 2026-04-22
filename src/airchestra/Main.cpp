#include "AppEngine.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

// Initialise JUCE's internal MessageManager before any JUCE audio/MIDI objects
// are created.  Without this, AudioDeviceManager may fail to open a device on
// macOS because it tries to dispatch onto a non-existent JUCE message thread.
#include <juce_events/juce_events.h>

int main(int argc, char* argv[])
{
    // JUCE init must come before QGuiApplication so the MessageManager exists
    // when HeadlessAudioEngine is constructed inside AppEngine.
    juce::initialiseJuce_GUI();

    QGuiApplication app(argc, argv);
    app.setApplicationName("Airchestra");
    app.setApplicationVersion("0.3.0");

    QQuickStyle::setStyle("Basic");

    airchestra::AppEngine engine;

    QQmlApplicationEngine qmlEngine;
    qmlEngine.rootContext()->setContextProperty("appEngine", &engine);
    qmlEngine.load(QUrl("qrc:/qml/main.qml"));

    if (qmlEngine.rootObjects().isEmpty())
        return -1;

    int result = app.exec();
    juce::shutdownJuce_GUI();
    return result;
}
