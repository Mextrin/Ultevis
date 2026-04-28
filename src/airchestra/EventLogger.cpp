#include "EventLogger.h"

#include <QCoreApplication>
#include <QDir>
#include <QTextStream>

namespace airchestra
{
EventLogger::EventLogger()
{
    const auto appDir = QCoreApplication::applicationDirPath();
    QDir logDir(appDir + "/logs");

    if (!logDir.exists())
        logDir.mkpath(".");

    const auto path = logDir.filePath("airchestra-events.jsonl");
    logFile.setFileName(path);
    ready = logFile.open(QIODevice::Append | QIODevice::Text);
    statusText = ready ? "Logging to " + path
                       : "Logging unavailable: could not open " + path;
}

void EventLogger::log(AppEventType eventType, EventFields fields)
{
    QMutexLocker locker(&mutex);
    if (!ready)
        return;

    QString line;
    QTextStream ts(&line);
    ts << "{\"timestamp\":\"" << escapeJson(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)) << "\"";
    ts << ",\"event\":\"" << toEventName(eventType) << "\"";

    for (const auto& [key, value] : fields)
        ts << ",\"" << escapeJson(key) << "\":\"" << escapeJson(value) << "\"";

    ts << "}\n";

    logFile.write(line.toUtf8());
    logFile.flush();
}

bool EventLogger::isReady() const noexcept
{
    return ready;
}

QString EventLogger::getStatusText() const
{
    QMutexLocker locker(&mutex);
    return statusText;
}

const char* EventLogger::toEventName(AppEventType eventType) noexcept
{
    switch (eventType)
    {
        case AppEventType::AppStarted: return "app_started";
        case AppEventType::MainWindowCreated: return "main_window_created";
        case AppEventType::LandingPageShown: return "landing_page_shown";
        case AppEventType::ButtonClicked: return "button_clicked";
        case AppEventType::ScreenChanged: return "screen_changed";
        case AppEventType::SessionStateChanged: return "session_state_changed";
        case AppEventType::AppClosing: return "app_closing";
    }
    return "unknown";
}

QString EventLogger::escapeJson(const QString& text)
{
    QString escaped;
    escaped.reserve(text.size());
    for (const QChar c : text)
    {
        if (c == '\\') escaped += "\\\\";
        else if (c == '"') escaped += "\\\"";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else if (c == '\t') escaped += "\\t";
        else escaped += c;
    }
    return escaped;
}
}
