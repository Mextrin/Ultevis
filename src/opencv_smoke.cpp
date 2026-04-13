#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <exception>
#include <iostream>
#include <string_view>

namespace
{
bool hasArgument(int argc, char** argv, std::string_view expected)
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::string_view{ argv[i] } == expected)
            return true;
    }

    return false;
}

cv::VideoCapture openCamera()
{
    // Prefer Windows-native camera backends for the II1305 Windows workflow.
    for (const int backend : { cv::CAP_MSMF, cv::CAP_DSHOW, cv::CAP_ANY })
    {
        cv::VideoCapture camera(0, backend);
        if (camera.isOpened())
            return camera;
    }

    return {};
}
}

int main(int argc, char** argv)
{
    try
    {
        std::cout << "OpenCV version: " << cv::getVersionString() << '\n';

        cv::Mat image(120, 360, CV_8UC3, cv::Scalar(24, 24, 24));
        cv::putText(image,
                    "II1305 OpenCV smoke test",
                    cv::Point(18, 68),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.7,
                    cv::Scalar(80, 220, 120),
                    2,
                    cv::LINE_AA);

        std::cout << "Created test image: " << image.cols << "x" << image.rows << '\n';

        if (!hasArgument(argc, argv, "--camera"))
        {
            std::cout << "Camera check skipped. Run with --camera to test webcam capture." << '\n';
            return 0;
        }

        cv::VideoCapture camera = openCamera();
        if (!camera.isOpened())
        {
            std::cerr << "Could not open webcam 0 with MSMF, DirectShow, or default backend." << '\n';
            return 2;
        }

        cv::Mat frame;
        if (!camera.read(frame) || frame.empty())
        {
            std::cerr << "Webcam opened, but no frame was captured." << '\n';
            return 3;
        }

        std::cout << "Captured webcam frame: " << frame.cols << "x" << frame.rows << '\n';
        return 0;
    }
    catch (const cv::Exception& error)
    {
        std::cerr << "OpenCV error: " << error.what() << '\n';
        return 10;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Unexpected error: " << error.what() << '\n';
        return 11;
    }
}
