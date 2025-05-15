cmake_minimum_required(VERSION 3.22)
project(ADHDPomodoro)

set(CMAKE_CXX_STANDARD 17)

# Explicitly set SFML paths
set(SFML_INCLUDE_DIR "C:/SFML/include")
set(SFML_LIBRARY_DIR "C:/SFML/lib")
set(SFML_DLL_DIR "${CMAKE_SOURCE_DIR}/dll") # Path to the DLLs

# Include SFML headers
include_directories(${SFML_INCLUDE_DIR})

# Add executable with all source files
add_executable(ADHDPomodoro 
    src/main.cpp
    src/Settings.cpp
)

# Link SFML libraries explicitly
target_link_libraries(ADHDPomodoro PRIVATE
    optimized "${SFML_LIBRARY_DIR}/sfml-graphics.lib" debug "${SFML_LIBRARY_DIR}/sfml-graphics-d.lib"
    optimized "${SFML_LIBRARY_DIR}/sfml-window.lib" debug "${SFML_LIBRARY_DIR}/sfml-window-d.lib"
    optimized "${SFML_LIBRARY_DIR}/sfml-system.lib" debug "${SFML_LIBRARY_DIR}/sfml-system-d.lib"
)

# Copy DLLs to the output directory
add_custom_command(TARGET ADHDPomodoro POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SFML_DLL_DIR}/sfml-graphics-d-2.dll"
    "${SFML_DLL_DIR}/sfml-window-d-2.dll"
    "${SFML_DLL_DIR}/sfml-system-d-2.dll"
    $<TARGET_FILE_DIR:ADHDPomodoro>
)