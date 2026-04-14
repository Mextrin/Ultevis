#include <opencv2/opencv.hpp>

void startCameraFeed() {
    cv::VideoCapture cap(0); // Open default camera

    if (!cap.isOpened()) {
        return;
    }

    cv::Mat frame;
    while (cap.read(frame)) {
        // Display the frame
        cv::imshow("Camera Feed", frame);
        
        // Press 'q' to quit
        if (cv::waitKey(1) == 'q') {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
}