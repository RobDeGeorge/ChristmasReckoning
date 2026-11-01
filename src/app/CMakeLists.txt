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
        qml/charts/HopProfile.qml
        qml/charts/LineChart.qml
        qml/charts/LogBarChart.qml
        qml/charts/ZoneGantt.qml
        qml/components/CalendarButton.qml
        qml/components/Formula.qml
        qml/components/LiveBadge.qml
        qml/components/ParamSlider.qml
        qml/components/PillButton.qml
        qml/components/PlayPauseButton.qml
        qml/components/Readout.qml
        qml/components/SnowField.qml
        qml/components/SourceChip.qml
        qml/components/StatusLight.qml
        qml/globe/Globe.qml
        qml/globe/SantaMarker.qml
        qml/mission/ConsoleDetail.qml
        qml/mission/ConsoleTile.qml
        qml/mission/LiveBanner.qml
        qml/mission/ScenarioDetail.qml
        qml/mission/SidePanel.qml
        qml/mission/SourcesDetail.qml
        qml/mission/Telemetry.qml
        qml/mission/TimelineBar.qml
        qml/pages/CountdownPage.qml
        qml/pages/MissionControlPage.qml
        qml/js/fmt.js
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
        shaders/snow.frag
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

# Fonts, and the console formulas typeset to SVG by tools/formulas/render.cjs.
file(GLOB cr_font_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/assets/fonts/*.ttf")
file(GLOB cr_formula_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/assets/formulas/*.svg")
qt_add_resources(christmasreckoning "assets"
    PREFIX "/"
    BASE "${PROJECT_SOURCE_DIR}/assets"
    FILES ${cr_font_files} ${cr_formula_files}
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

if(EMSCRIPTEN)
    # Smallest code; the app only decodes JPEG (Earth imagery), PNG (built in) and SVG (formulas).
    target_link_options(christmasreckoning PRIVATE -Oz)
    qt_import_plugins(christmasreckoning
        INCLUDE_BY_TYPE imageformats Qt::QJpegPlugin Qt::QSvgPlugin
    )
endif()
