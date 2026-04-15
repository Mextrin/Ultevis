cmake_minimum_required(VERSION 3.20)
project(Airchestra VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)
add_subdirectory(sfizz)

juce_add_console_app(Airchestra)

target_sources(Airchestra PRIVATE 
    src/main.cpp
    src/Audio/AudioEngine.cpp
    src/Camera/CameraFeed.cpp
)

target_link_libraries(Airchestra PRIVATE
    juce::juce_core
    juce::juce_audio_devices
    juce::juce_audio_basics
    sfizz_static
)

target_include_directories(
    Airchestra PRIVATE src
)