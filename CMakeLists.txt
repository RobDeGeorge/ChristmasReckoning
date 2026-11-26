cmake_minimum_required(VERSION 3.24)

project(ChristmasReckoning
    VERSION 2.0.0
    DESCRIPTION "Santa tracker: his flight path, live on Christmas Eve"
    HOMEPAGE_URL "https://www.christmasreckoning.com"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The physics core and its tests only need QtCore/QtTest, so CI can test them without GUI libraries.
option(CR_BUILD_APP "Build the Qt Quick application" ON)
option(CR_BUILD_TESTS "Build the unit tests" ON)

set(cr_qt_components Core)
if(CR_BUILD_APP)
    list(APPEND cr_qt_components Gui Qml Quick Network ShaderTools)
endif()
if(CR_BUILD_TESTS AND NOT EMSCRIPTEN)
    list(APPEND cr_qt_components Test)
endif()
find_package(Qt6 6.12 REQUIRED COMPONENTS ${cr_qt_components})
qt_standard_project_setup(REQUIRES 6.12)

add_subdirectory(src/core)

if(CR_BUILD_APP)
    add_subdirectory(src/app)
endif()

if(CR_BUILD_TESTS AND NOT EMSCRIPTEN)
    enable_testing()
    add_subdirectory(tests)
endif()
