cmake_minimum_required(VERSION 3.22)
project(ADHDPomodoro)

set(CMAKE_CXX_STANDARD 17)

# Option to build CLI version only (useful for systems without SFML)
option(BUILD_CLI_ONLY "Build only the CLI version" OFF)

# CLI version (cross-platform, no dependencies)
add_executable(ADHDPomodoro-CLI 
    src/cli_main.cpp
    src/CLISettings.cpp
)

# Link required libraries for CLI version
if(WIN32)
    # No additional libraries needed for Windows
else()
    # For Linux, might need pthread
    find_package(Threads REQUIRED)
    target_link_libraries(ADHDPomodoro-CLI PRIVATE Threads::Threads)
endif()

# GUI version (only if not CLI-only and SFML is available)
if(NOT BUILD_CLI_ONLY)
    # Try to find SFML
    find_package(SFML 2.5 COMPONENTS graphics window system QUIET)
    
    if(SFML_FOUND)
        message(STATUS "SFML found, building GUI version")
        
        # Add GUI executable
        add_executable(ADHDPomodoro 
            src/main.cpp
            src/Settings.cpp
        )
        
        # Link SFML libraries
        target_link_libraries(ADHDPomodoro PRIVATE sfml-graphics sfml-window sfml-system)
        
        # Windows-specific settings
        if(WIN32)
            target_link_libraries(ADHDPomodoro PRIVATE user32)
        endif()
        
    else()
        # Fallback to explicit SFML paths (for Windows development)
        if(WIN32 AND EXISTS "C:/SFML/include")
            message(STATUS "Using explicit SFML paths for Windows")
            
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
        else()
            message(STATUS "SFML not found, building CLI version only")
        endif()
    endif()
endif()

# Print build information
if(TARGET ADHDPomodoro)
    message(STATUS "Building both GUI and CLI versions")
else()
    message(STATUS "Building CLI version only")
endif()