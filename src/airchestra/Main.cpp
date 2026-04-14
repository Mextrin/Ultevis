#include "AppEngine.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("Airchestra");
    app.setApplicationVersion("0.2.0");

    QQuickStyle::setStyle("Basic");

    airchestra::AppEngine engine;

    QQmlApplicationEngine qmlEngine;
    qmlEngine.rootContext()->setContextProperty("appEngine", &engine);
    qmlEngine.load(QUrl("qrc:/qml/main.qml"));

    if (qmlEngine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
