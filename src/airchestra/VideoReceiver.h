#pragma once
#include <QObject>
#include <QImage>
#include <QTimer>
#include <QStandardPaths>
#include <QQuickImageProvider>

namespace airchestra {

// 1. The Provider that hands the image to QML
class CameraImageProvider : public QQuickImageProvider {
public:
    CameraImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {
        currentImage = QImage(640, 480, QImage::Format_RGB888);
        currentImage.fill(Qt::black);
    }

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override {
        if (size) *size = currentImage.size();
        return currentImage;
    }

    void updateImage(const QImage& img) { currentImage = img; }

private:
    QImage currentImage;
};

// 2. The Receiver that reads the Atomic Temp File
class VideoReceiver : public QObject {
    Q_OBJECT
public:
    VideoReceiver(CameraImageProvider* provider, QObject* parent = nullptr) 
        : QObject(parent), imgProvider(provider) 
    {
        // Get the exact same cross-platform Temp Directory Python is using
        framePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/airchestra_frame.jpg";
        
        connect(&timer, &QTimer::timeout, this, &VideoReceiver::grabFrame);
        timer.start(33); // Check for new frames ~30 times a second
    }

private slots:
    void grabFrame() {
        QImage img;
        // QImage::load safely reads the file and handles the JPEG format instantly
        if (img.load(framePath)) {
            imgProvider->updateImage(img.convertToFormat(QImage::Format_RGB888));
        }
    }

private:
    QString framePath;
    QTimer timer;
    CameraImageProvider* imgProvider;
};

} // namespace airchestra