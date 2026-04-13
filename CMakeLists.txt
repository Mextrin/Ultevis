cmake_minimum_required(VERSION 3.25)

project(II1305GestureInstrument VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio highgui)
find_package(imgui CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
add_subdirectory(external/JUCE)

add_executable(opencv_smoke
    src/opencv_smoke.cpp
)

target_compile_features(opencv_smoke PRIVATE cxx_std_20)
target_include_directories(opencv_smoke PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(opencv_smoke PRIVATE ${OpenCV_LIBS})

if (MSVC)
    target_compile_options(opencv_smoke PRIVATE /W4 /permissive-)
else()
    target_compile_options(opencv_smoke PRIVATE -Wall -Wextra -Wpedantic)
endif()

juce_add_gui_app(Airchestra
    PRODUCT_NAME "Airchestra"
)

set_target_properties(Airchestra
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
)

target_sources(Airchestra
    PRIVATE
        src/airchestra/EventLogger.cpp
        src/airchestra/ImGuiLayer.cpp
        src/airchestra/LandingPageView.cpp
        src/airchestra/Main.cpp
        src/airchestra/MainComponent.cpp
        src/airchestra/OverlayView.cpp
        src/airchestra/UiActions.cpp
        src/airchestra/UiRenderer.cpp
)

target_compile_features(Airchestra PRIVATE cxx_std_20)

target_compile_definitions(Airchestra
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Airchestra,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Airchestra,JUCE_VERSION>"
)

target_link_libraries(Airchestra
    PRIVATE
        imgui::imgui
        OpenGL::GL
        juce::juce_gui_extra
        juce::juce_opengl
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
