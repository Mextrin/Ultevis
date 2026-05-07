#include <QCoreApplication>
#include <QDir>
#include <QString>

namespace airchestra {

    inline QString runtimePath(const QString& folderName) {
        QDir appDir(QCoreApplication::applicationDirPath());
        
        return appDir.absoluteFilePath(folderName);
    }

} // namespace airchestra