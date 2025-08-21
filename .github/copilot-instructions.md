# ADHD Pomodoro Timer

ADHD Pomodoro Timer is a Windows-specific C++ application built with SFML that provides an always-on-top timer widget designed for individuals with ADHD. The application features customizable focus and rest phases, feedback systems, and session logging.

Always reference these instructions first and fallback to search or bash commands only when you encounter unexpected information that does not match the info here.

## Working Effectively

### ⚠️ CRITICAL PLATFORM REQUIREMENTS
- **Windows-only**: This application CANNOT be built or run on Linux or macOS due to Windows API usage (windows.h, HWND, SetWindowPos).
- **Required OS**: Windows 11 (as specified in README.md)
- **Do not attempt to build on non-Windows systems** - it will fail with `windows.h: No such file or directory`

### Prerequisites and Setup
- Install Visual Studio with MSVC C++ compiler
- Download and install SFML 2.5.1 or later for MSVC from https://www.sfml-dev.org/download.php
- Extract SFML to `C:/SFML/` (or update paths in CMakeLists.txt)
- Install CMake (minimum version 3.22)
- Ensure the following SFML directory structure exists:
  ```
  C:/SFML/
  ├── include/
  ├── lib/
  └── bin/
  ```

### Building the Application
- **NEVER CANCEL**: Build takes 2-5 minutes on typical Windows systems. Set timeout to 10+ minutes.
- Create and configure build directory:
  ```
  mkdir build
  cd build
  cmake -G "Visual Studio 17 2022" -A x64 ..
  ```
- Build the project:
  ```
  cmake --build . --config Debug
  ```
  or
  ```
  cmake --build . --config Release
  ```

### Required DLLs
The application requires SFML DLLs in the same directory as the executable:
- `sfml-graphics-d-2.dll` (Debug) or `sfml-graphics-2.dll` (Release)
- `sfml-window-d-2.dll` (Debug) or `sfml-window-2.dll` (Release)  
- `sfml-system-d-2.dll` (Debug) or `sfml-system-2.dll` (Release)

Copy these from `C:/SFML/bin/` to your build output directory, or create a `dll/` folder in the project root with these files.

### Running the Application
- Navigate to build output directory (e.g., `build/Debug/` or `build/Release/`)
- Ensure SFML DLLs are present in the same directory
- Run `ADHDPomodoro.exe`
- The application creates two files during operation:
  - `settings.txt` - Configuration file (focus/rest durations)
  - `log.txt` - Session logging file

## Validation Scenarios

### Always Test These User Scenarios After Changes:
1. **Application Launch**: 
   - Launch ADHDPomodoro.exe and verify the timer window appears
   - Window should be always-on-top
   - Timer should display "Click Start to begin"

2. **Basic Timer Flow**:
   - Click "Start" button to begin focus session
   - Wait for focus timer countdown (default 10 minutes, but can be set to 1 minute for testing)
   - Verify transition to "Extended Focus" state after timer expires
   - Press 'G', 'B', 'O', or 'H' keys during Extended Focus
   - If 'H' pressed, verify transition to Hyperfocus state
   - In Hyperfocus, press 'R' to start rest phase
   - During rest, press 'F' to start new focus cycle

3. **Settings Management**:
   - Click "Settings" in menu bar to open settings window
   - Use +/- buttons to adjust focus and rest durations
   - Click "Save" to persist settings
   - Verify `settings.txt` file is created/updated
   - Close and restart application to verify settings persistence

4. **Session Logging**:
   - Complete at least one full focus/rest cycle
   - Verify `log.txt` contains timestamped session entries
   - Check that different session types are logged (Focus, Rest, Hyperfocus, etc.)

### Build Validation Commands
- **NEVER CANCEL**: Always wait for builds to complete. Set timeout to 15+ minutes.
- After any code changes, always rebuild:
  ```
  cd build
  cmake --build . --config Debug
  ```
- No automated tests exist - validation is manual through the GUI scenarios above.

## Common Issues and Solutions

### Build Problems
- **SFML not found**: Verify SFML paths in CMakeLists.txt match your installation
- **Missing DLLs**: Copy SFML DLLs to executable directory
- **Wrong Visual Studio version**: Update CMake generator to match your VS installation
- **x86 vs x64 mismatch**: Ensure consistent architecture between SFML build and project

### Runtime Problems  
- **Application doesn't start**: Missing SFML DLLs in executable directory
- **Settings not saving**: Verify write permissions in application directory
- **Timer not staying on top**: Windows-specific SetWindowPos API issue, requires Windows

## Code Architecture

### Key Components
- **main.cpp**: Application entry point, main game loop, state management, Windows API integration
- **Settings.h/cpp**: Configuration management, settings GUI window, file I/O
- **State enum**: Focus, ExtendedFocus, Rest, Hyperfocus, IdleAfterRest
- **Timer states**: Uses std::chrono for timing, SFML for rendering

### Important Code Locations
- Windows API usage: `src/main.cpp` lines 52-53 (SetWindowPos for always-on-top)
- State machine: `src/main.cpp` main() function switch statement
- Settings file I/O: `src/Settings.cpp` loadSettings()/saveSettings()
- GUI rendering: Throughout main.cpp and Settings.cpp using SFML

### Configuration Files
- `settings.txt`: Focus and rest durations (in minutes)
- `log.txt`: Session logging with timestamps
- `CMakeLists.txt`: Hardcoded SFML paths for Windows

## Current Status
- **Under construction**: Settings panel implementation is ongoing
- **Known limitation**: Windows-only due to Windows API dependencies
- **No cross-platform support**: Would require significant refactoring to replace Windows API calls

## Development Notes
- When making changes to UI elements, always test with actual GUI interaction
- Session logging is append-only - log.txt will grow over time
- Settings are loaded at startup and saved on demand
- The application uses a simple state machine for timer phases
- SFML provides graphics, Windows API provides always-on-top functionality

## DO NOT ATTEMPT
- Building on Linux/macOS systems (will fail)
- Cross-platform port without extensive refactoring
- Running without required SFML DLLs
- Automated testing (no test framework exists)