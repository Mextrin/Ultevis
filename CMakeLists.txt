cmake_minimum_required(VERSION 3.20)
project(Airchestra VERSION 1.0.0)

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

add_subdirectory(JUCE)
set(SFIZZ_GIT_SUBMODULE_CHECK OFF CACHE BOOL "Disable sfizz's git submodule check in superbuilds" FORCE)
add_subdirectory(sfizz)

juce_add_console_app(Airchestra)

target_sources(Airchestra PRIVATE 
    src/main.cpp
    src/Audio/AudioEngine.cpp
    src/Audio/OscillatorVoice.cpp
    src/App/AppController.cpp
    src/Input/TerminalUI.cpp
    src/Camera/CameraFeed.cpp
)

target_link_libraries(Airchestra PRIVATE
    juce::juce_core
    juce::juce_audio_devices
    juce::juce_audio_basics
    sfizz::sfizz
)

target_include_directories(
    Airchestra PRIVATE src
)
