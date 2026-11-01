# Native command-line tools that run at build/package time (never shipped to the browser).
add_executable(cr-report cr-report.cpp)
target_link_libraries(cr-report PRIVATE cr_core Qt6::Core)
target_compile_definitions(cr-report PRIVATE CR_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
