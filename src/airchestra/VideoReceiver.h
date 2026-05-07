#pragma once
#include <QObject>
#include <QImage>
#include <QTimer>
#include <QSaveFile>
#include <QStandardPaths>
#include <QQuickImageProvider>

namespace airchestra {

inline QString cameraFramePath() {
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/airchestra_frame.jpg";
}

inline QImage makeBlackCameraFrame() {
    QImage image(640, 360, QImage::Format_RGB888);
    image.fill(Qt::black);
    return image;
}

inline bool writeBlackCameraFrame() {
    QSaveFile frameFile(cameraFramePath());
    if (!frameFile.open(QIODevice::WriteOnly)) {
        return false;
    }

    const QImage blackFrame = makeBlackCameraFrame();
    if (!blackFrame.save(&frameFile, "JPG", 95)) {
        frameFile.cancelWriting();
        return false;
    }

    return frameFile.commit();
}

// The Provider that hands the image to QML
class CameraImageProvider : public QQuickImageProvider {
public:
    CameraImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {
        currentImage = makeBlackCameraFrame();
    }

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override {
        if (size) *size = currentImage.size();
        return currentImage;
    }

    void updateImage(const QImage& img) { currentImage = img; }
    void resetToBlack() { currentImage = makeBlackCameraFrame(); }

private:
    QImage currentImage;
};

// The Receiver that reads the Atomic Temp File
class VideoReceiver : public QObject {
    Q_OBJECT
public:
    VideoReceiver(CameraImageProvider* provider, QObject* parent = nullptr) 
        : QObject(parent), imgProvider(provider) 
    {
        framePath = cameraFramePath();
        writeBlackCameraFrame();
        if (imgProvider != nullptr) {
            imgProvider->resetToBlack();
        }
        
        connect(&timer, &QTimer::timeout, this, &VideoReceiver::grabFrame);
        timer.start(33); // Check for new frames ~30 times a second
    }

private slots:
    void grabFrame() {
        QImage img;
        if (img.load(framePath)) {
            imgProvider->updateImage(img.convertToFormat(QImage::Format_RGB888));
        }
    }

private:
    QString framePath;
    QTimer timer;
    CameraImageProvider* imgProvider;
};

}
