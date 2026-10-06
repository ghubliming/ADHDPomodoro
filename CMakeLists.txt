cmake_minimum_required(VERSION 3.16)
project(ADHDPomodoro VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

# The terminal app and tests build everywhere with no dependencies.
# The old SFML widget (src/gui) builds only when SFML is found and this is OFF.
option(BUILD_CLI_ONLY "Skip the SFML GUI even if SFML is available" OFF)
option(ADHD_BUILD_TESTS "Build the unit tests" ON)

# Warnings for our own targets.
add_library(adhd_warnings INTERFACE)
if(MSVC)
    target_compile_options(adhd_warnings INTERFACE /W4 /utf-8 /permissive-)
    target_compile_definitions(adhd_warnings INTERFACE _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    target_compile_options(adhd_warnings INTERFACE -Wall -Wextra -Wpedantic -Wshadow)
endif()

# ---- core logic: no I/O beyond plain files, fully unit-tested
add_library(adhd_core STATIC
    src/core/Config.cpp
    src/core/Engine.cpp
    src/core/Journal.cpp
    src/core/ParkingLot.cpp
    src/core/Paths.cpp
    src/core/Phase.cpp
    src/core/Stats.cpp
    src/core/TimeUtil.cpp
)
target_include_directories(adhd_core PUBLIC src)
target_link_libraries(adhd_core PUBLIC adhd_warnings)

# ---- terminal app
add_executable(adhd-pomodoro
    src/cli/App.cpp
    src/cli/Render.cpp
    src/cli/StatsView.cpp
    src/cli/Terminal.cpp
    src/cli/main.cpp
)
target_link_libraries(adhd-pomodoro PRIVATE adhd_core)
target_compile_definitions(adhd-pomodoro PRIVATE ADHD_VERSION="${PROJECT_VERSION}")
install(TARGETS adhd-pomodoro RUNTIME DESTINATION bin)

# ---- tests
if(ADHD_BUILD_TESTS)
    enable_testing()
    add_executable(adhd_tests tests/test_main.cpp src/cli/Render.cpp)
    target_link_libraries(adhd_tests PRIVATE adhd_core)
    add_test(NAME adhd_tests COMMAND adhd_tests)
endif()

# ---- legacy SFML GUI (Windows, always-on-top widget)
if(NOT BUILD_CLI_ONLY)
    find_package(SFML 2.5 COMPONENTS graphics window system QUIET)
    if(SFML_FOUND)
        message(STATUS "SFML found, building the GUI widget")
        add_executable(ADHDPomodoro src/gui/main.cpp src/gui/Settings.cpp)
        target_link_libraries(ADHDPomodoro PRIVATE sfml-graphics sfml-window sfml-system)
        if(WIN32)
            target_link_libraries(ADHDPomodoro PRIVATE user32)
        endif()
    elseif(WIN32 AND EXISTS "C:/SFML/include")
        message(STATUS "Using explicit SFML paths for Windows")
        set(SFML_INCLUDE_DIR "C:/SFML/include")
        set(SFML_LIBRARY_DIR "C:/SFML/lib")
        set(SFML_DLL_DIR "${CMAKE_SOURCE_DIR}/dll")
        add_executable(ADHDPomodoro src/gui/main.cpp src/gui/Settings.cpp)
        target_include_directories(ADHDPomodoro PRIVATE ${SFML_INCLUDE_DIR})
        target_link_libraries(ADHDPomodoro PRIVATE
            optimized "${SFML_LIBRARY_DIR}/sfml-graphics.lib" debug "${SFML_LIBRARY_DIR}/sfml-graphics-d.lib"
            optimized "${SFML_LIBRARY_DIR}/sfml-window.lib" debug "${SFML_LIBRARY_DIR}/sfml-window-d.lib"
            optimized "${SFML_LIBRARY_DIR}/sfml-system.lib" debug "${SFML_LIBRARY_DIR}/sfml-system-d.lib"
        )
        add_custom_command(TARGET ADHDPomodoro POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SFML_DLL_DIR}/sfml-graphics-d-2.dll"
            "${SFML_DLL_DIR}/sfml-window-d-2.dll"
            "${SFML_DLL_DIR}/sfml-system-d-2.dll"
            $<TARGET_FILE_DIR:ADHDPomodoro>
        )
    else()
        message(STATUS "SFML not found, skipping the GUI widget")
    endif()
endif()
