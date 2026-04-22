cmake_minimum_required(VERSION 3.25)

project(Airchestra VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ---------------------------------------------------------------------------
# Qt
# ---------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Multimedia)

# ---------------------------------------------------------------------------
# JUCE  (use local submodule if initialised, otherwise fetch)
# ---------------------------------------------------------------------------
include(FetchContent)

if(EXISTS "${CMAKE_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE)
else()
    message(STATUS "JUCE submodule not found — fetching via FetchContent (one-time download)")
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.6
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
qt_add_executable(Airchestra
    src/airchestra/main.cpp
    src/airchestra/AppEngine.cpp
    src/airchestra/EventLogger.cpp
    src/airchestra/ViewState.h

    src/Core/GlobalState.h
    src/Audio/AudioEngine.cpp
    src/Audio/OscillatorVoice.cpp
    src/Camera/CameraFeed.cpp

    resources.qrc
)

target_include_directories(Airchestra PRIVATE src)

target_compile_definitions(Airchestra PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

target_link_libraries(Airchestra PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    juce::juce_core
    juce::juce_events
    juce::juce_audio_devices
    juce::juce_audio_basics
)

# ---------------------------------------------------------------------------
# macOS bundle settings
# ---------------------------------------------------------------------------
if(APPLE)
    set_target_properties(Airchestra PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "se.kth.ii1305.airchestra"
        MACOSX_BUNDLE_BUNDLE_NAME "Airchestra"
        MACOSX_BUNDLE_COPYRIGHT "KTH II1305 Airchestra"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/Info.plist.in"
    )

    # Re-sign to bind the Info.plist (NSCameraUsageDescription) into the signature,
    # which macOS requires before granting camera access.
    add_custom_command(TARGET Airchestra POST_BUILD
        COMMAND codesign --deep --force --sign - "$<TARGET_BUNDLE_DIR:Airchestra>"
        COMMENT "Re-signing Airchestra.app"
    )
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
    add_compile_options(/FS)
endif()
