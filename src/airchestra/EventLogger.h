#pragma once

#include <QDateTime>
#include <QFile>
#include <QMutex>
#include <QString>

#include <initializer_list>
#include <utility>

namespace airchestra
{
enum class AppEventType
{
    AppStarted,
    MainWindowCreated,
    LandingPageShown,
    ButtonClicked,
    ScreenChanged,
    SessionStateChanged,
    AppClosing
};

using EventFields = std::initializer_list<std::pair<QString, QString>>;

class EventLogger final
{
public:
    EventLogger();

    void log(AppEventType eventType, EventFields fields = {});

    bool isReady() const noexcept;
    QString getStatusText() const;

private:
    static const char* toEventName(AppEventType eventType) noexcept;
    static QString escapeJson(const QString& text);

    mutable QMutex mutex;
    QFile logFile;
    QString statusText;
    bool ready = false;
};
}
