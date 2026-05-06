#pragma once

#include <QCoreApplication>
#include <QString>

namespace airchestra {

inline QString runtimeBasePath()
{
    return QCoreApplication::applicationDirPath();
}

inline QString runtimePath(const QString& relativePath)
{
    QString base = runtimeBasePath();
    return base + "/" + relativePath;
}

} // namespace airchestra
