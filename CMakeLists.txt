cmake_minimum_required(VERSION 3.20)
project(Airchestra VERSION 1.0.0)

set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose Release to make audio fast" FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        CACHE STRING "Use the static MSVC runtime across the superbuild" FORCE)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>"
        CACHE STRING "Use embedded debug info to avoid shared PDB contention" FORCE)
    add_compile_options(/FS)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(SFIZZ_TESTS OFF CACHE BOOL "" FORCE)
set(SFIZZ_JACK OFF CACHE BOOL "" FORCE)
set(SFIZZ_RENDER OFF CACHE BOOL "" FORCE)
set(SFIZZ_BENCHMARKS OFF CACHE BOOL "" FORCE)

if(MSVC)
    set(SFIZZ_USE_SYSTEM_CXXOPTS ON CACHE BOOL "" FORCE)
else()
    set(SFIZZ_USE_SYSTEM_CXXOPTS OFF CACHE BOOL "" FORCE)
    set(SFIZZ_SHARED OFF CACHE BOOL "" FORCE)
    set(SFIZZ_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
endif()

add_subdirectory(JUCE)
add_subdirectory(sfizz)

juce_add_console_app(Airchestra)

target_sources(Airchestra PRIVATE 
    src/main.cpp
    src/Audio/AudioEngine.cpp
    src/Audio/OscillatorVoice.cpp
    src/Camera/CameraFeed.cpp

    src/airchestra/AppEngine.cpp
    src/airchestra/EventLogger.cpp

    src/airchestra/AppEngine.h
    src/airchestra/ViewState.h
    src/airchestra/EventLogger.h
    src/airchestra/VideoReceiver.h
    
    src/Core/GlobalState.h
    src/Audio/AudioEngine.h

    resources.qrc
)

target_link_libraries(Airchestra PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    juce::juce_core
    juce::juce_audio_devices
    juce::juce_audio_basics
    sfizz::sfizz
    "$<$<PLATFORM_ID:Darwin>:-framework CoreAudio>"
)

target_include_directories(
    Airchestra PRIVATE src
)
