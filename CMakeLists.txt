cmake_minimum_required(VERSION 3.22)
project(ADHDPomodoro)

set(CMAKE_CXX_STANDARD 17)

# Explicitly set SFML paths
set(SFML_INCLUDE_DIR "C:/SFML/include")
set(SFML_LIBRARY_DIR "C:/SFML/lib")

# Include SFML headers
include_directories(${SFML_INCLUDE_DIR})

# Add executable
add_executable(ADHDPomodoro src/main.cpp)

# Link SFML libraries explicitly
target_link_libraries(ADHDPomodoro PRIVATE
    optimized "C:/SFML/lib/sfml-graphics.lib" debug "C:/SFML/lib/sfml-graphics-d.lib"
    optimized "C:/SFML/lib/sfml-window.lib" debug "C:/SFML/lib/sfml-window-d.lib"
    optimized "C:/SFML/lib/sfml-system.lib" debug "C:/SFML/lib/sfml-system-d.lib"
)

# Ensure library directory is included
link_directories(${SFML_LIBRARY_DIR})