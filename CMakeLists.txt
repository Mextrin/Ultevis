cmake_minimum_required(VERSION 3.25)

project(Airchestra VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Multimedia)

qt_add_executable(Airchestra
    src/airchestra/main.cpp
    src/airchestra/AppEngine.cpp
    src/airchestra/EventLogger.cpp
    src/airchestra/ViewState.h
    resources.qrc
)

target_link_libraries(Airchestra PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
)

if(APPLE)
    set_target_properties(Airchestra PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "se.kth.ii1305.airchestra"
        MACOSX_BUNDLE_BUNDLE_NAME "Airchestra"
        MACOSX_BUNDLE_COPYRIGHT "KTH II1305 Airchestra"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/Info.plist.in"
    )

    # Re-sign the bundle so the Info.plist (containing NSCameraUsageDescription,
    # etc.) is bound into the signature. Without this, macOS silently denies
    # camera access because it can't trust the usage description.
    add_custom_command(TARGET Airchestra POST_BUILD
        COMMAND codesign --deep --force --sign - "$<TARGET_BUNDLE_DIR:Airchestra>"
        COMMENT "Re-signing Airchestra.app (binds Info.plist into signature)"
    )
endif()
