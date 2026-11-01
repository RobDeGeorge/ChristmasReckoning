qt_add_executable(christmasreckoning main.cpp)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(christmasreckoning
    URI ChristmasReckoning
    VERSION 1.0
    SOURCES
        astro.h astro.cpp
        countdownclock.h countdownclock.cpp
        mission.h mission.cpp
        platform.h platform.cpp
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/globe/Globe.qml
        qml/pages/CountdownPage.qml
        qml/pages/MissionControlPage.qml
)

if(EMSCRIPTEN)
    target_sources(christmasreckoning PRIVATE platform_wasm.cpp)
else()
    target_sources(christmasreckoning PRIVATE platform_native.cpp)
    target_compile_definitions(christmasreckoning PRIVATE CR_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
endif()

# GLSL ES 3.00 for WebGL 2 and GLSL 1.50 for desktop OpenGL; SPIR-V is always generated (Vulkan).
qt_add_shaders(christmasreckoning "shaders"
    PREFIX "/"
    GLSL "300es,150"
    NOHLSL
    NOMSL
    FILES
        shaders/globe.frag
)

# The simulation's world data and the matching delivery texture are built together by
# tools/data/build_world.py, so they ship together inside the app.
qt_add_resources(christmasreckoning "worlddata"
    PREFIX "/"
    BASE "${PROJECT_SOURCE_DIR}"
    FILES
        "${PROJECT_SOURCE_DIR}/data/world.bin"
        "${PROJECT_SOURCE_DIR}/data/delivery.png"
)

file(GLOB cr_font_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/assets/fonts/*.ttf")
qt_add_resources(christmasreckoning "assets"
    PREFIX "/"
    BASE "${PROJECT_SOURCE_DIR}/assets"
    FILES ${cr_font_files}
)

target_link_libraries(christmasreckoning PRIVATE
    cr_core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    Qt6::Network
)

if(NOT MSVC)
    target_compile_options(christmasreckoning PRIVATE -Wall -Wextra)
endif()
