cmake_minimum_required(VERSION 3.20)
project(Airchestra VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(OpenCV REQUIRED)
add_subdirectory(JUCE)

juce_add_gui_app(Airchestra
    PRODUCT_NAME "Airchestra"
    BUNDLE_ID "com.mextrin.airchestra"
    CAMERA_PERMISSION_ENABLED TRUE
    CAMERA_PERMISSION_TEXT "Ultevis needs camera access to read the MacBook camera feed."
)

target_sources(Airchestra PRIVATE 
    src/main.cpp
    src/Audio/AudioEngine.cpp
    src/Camera/CameraFeed.cpp
)

target_link_libraries(Airchestra PRIVATE
    juce::juce_core
    juce::juce_audio_devices
    juce::juce_audio_basics
    ${OpenCV_LIBS}
)

target_include_directories(
    Airchestra PRIVATE src
    ${OpenCV_INCLUDE_DIRS}
)
