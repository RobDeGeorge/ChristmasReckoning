# Physics/simulation core. Standard C++ plus QtCore only (no GUI), so it can be unit-tested anywhere.
add_library(cr_core STATIC
    analysis.h analysis.cpp
    atmosphere.h atmosphere.cpp
    calendar.h
    constants.h
    countdown.h countdown.cpp
    format.h format.cpp
    physics.h physics.cpp
    qtcalendar.h qtcalendar.cpp
    scenario.h scenario.cpp
    schedule.h schedule.cpp
    solar.h solar.cpp
    sources.h sources.cpp
    timeline.h timeline.cpp
    units.h
    worlddata.h worlddata.cpp
)
target_include_directories(cr_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cr_core PUBLIC Qt6::Core)
target_compile_features(cr_core PUBLIC cxx_std_20)
if(NOT MSVC)
    target_compile_options(cr_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
