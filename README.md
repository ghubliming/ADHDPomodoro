**Under construction**

Settings panel not working yet
[[CHANGELOG.md]](CHANGELOG.md)

Practice C++
- Build a database from scratch
- GUI from Scratch
- Connection with other apps
  - Consider creating a song playlist generator that provides calming music for studying, phonk music for energizing moments, and relaxing or happy music for rest, then repeats the cycle. 
  - Include a feature for controlling the length of the music or playlist. 
  - Allow dynamic adjustments based on user feedback while they are focusing.

# ADHD Pomodoro Timer -- Basic Ideas

## Overview
The ADHD Pomodoro Timer is a productivity tool designed specifically for individuals with ADHD. Built using C++ and the SFML library, this application adapts the traditional Pomodoro technique to better suit ADHD needs by offering customizable focus and rest phases, an always-on-top timer widget, and detailed logging for productivity tracking. The goal is to help users manage their attention spans and breaks effectively.

## Features
- **Always-on-Top Timer Widget**: A small, persistent window that stays visible over other applications, displaying the current timer state.
- **ADHD-Focused States**:
  - **Focus**: A countdown timer for initial focused work.
  - **Extended Focus**: Automatically triggers if no input is given after the focus phase, supporting continued concentration.
  - **Rest**: A break period with options to provide feedback for adjusting future phases.
  - **Hyperfocus**: An optional mode for longer focus sessions, followed by a proportional rest period.
  - **Idle After Rest**: Tracks time spent idle after a rest phase, encouraging the next focus session.
- **Feedback System**: After focus or hyperfocus, users can press keys (`G` for Good, `B` for Bad, `O` for Okay) to rate their session, dynamically tweaking phase durations.
- **Configurable Settings**: Phase durations and other options are adjustable via a `settings.txt` file.
- **Logging**: All sessions (focus, rest, hyperfocus, idle) are logged with timestamps in a file for later analysis.

## Code Ideas
The project is built with an object-oriented design in C++ to keep the code modular and maintainable. Here’s how it’s structured:
- **Timer**: Handles countdowns and positive timers for tracking different states.
- **Phase**: Encapsulates a single phase (e.g., focus or rest) with its duration and behavior.
- **PomodoroManager**: Manages the flow of Pomodoro cycles, state transitions, and phase adjustments based on feedback.
- **Settings**: Loads and saves configuration data from a text file, making customization straightforward.
- **Logger**: Records session details (state, duration, timestamp) to a log file.
- **UserInterface**: Uses SFML to create the graphical interface, including window management, text rendering, and user input handling.

The use of **SFML** keeps the GUI lightweight and responsive, ideal for a small utility like this. The always-on-top feature is implemented with a mix of SFML and Windows API calls, ensuring the timer remains accessible at all times.

## Installation

### CLI Version (Cross-Platform)
1. **Prerequisites**:
   - A C++ compiler (GCC, Clang, or MSVC)
   - CMake (version 3.22 or later)

2. **Steps**:
   ```bash
   git clone https://github.com/ghubliming/ADHDPomodoro.git
   cd ADHDPomodoro
   mkdir build && cd build
   cmake .. -DBUILD_CLI_ONLY=ON
   make  # On Windows: cmake --build .
   ```

3. **Running**:
   ```bash
   ./ADHDPomodoro-CLI  # On Windows: ADHDPomodoro-CLI.exe
   ```

### GUI Version (Windows)
1. **Prerequisites**:
   - A C++ IDE like [CLion](https://www.jetbrains.com/clion/).
   - [SFML](https://www.sfml-dev.org/download.php) (version 2.5.1 or later) for MSVC.
   - Windows 11 (currently Windows-specific due to API usage).

2. **Steps**:
   - Clone the repository
   - Open the project in your IDE.
   - Set up CMake with MSVC and point `SFML_ROOT` to your SFML installation folder.
   - Build the project to create the executable.

3. **Running**:
   - Launch the `.exe` from the build directory.
   - Ensure SFML DLLs are in the same folder as the executable.

## Usage

### CLI Version
The command line version provides the same functionality as the GUI version with keyboard controls:

**Getting Started:**
```bash
./ADHDPomodoro-CLI
```

**Controls:**
- `s/S` - Start timer
- `p/P` - Pause/Resume timer  
- `f/F` - Finish current session
- `c/C` - Show/Edit settings
- `h/H` - Show help
- `q/Q` - Quit

**During Extended Focus:**
- `g/G` - Good session (go to rest)
- `b/B` - Bad session (go to rest)
- `o/O` - Okay session (go to rest)
- `h/H` - Enter Hyperfocus mode

**During Hyperfocus:**
- `r/R` - Go to rest

**During Rest/Idle:**
- `f/F` - Start new focus session

**Settings:**
- Focus and rest durations can be adjusted through the settings menu (`c/C`)
- Settings are automatically saved to `settings.txt`
- All sessions are logged to `log.txt` with timestamps

### GUI Version
The graphical version provides the same timer functionality with an always-on-top window interface:

- **Start**: Run the app to begin a focus session.
- **Interact**:
  - In Extended Focus: Press `G` (Good), `B` (Bad), `O` (Okay), or `H` (Hyperfocus).
  - In Hyperfocus: Press `R` to start rest.
  - After Rest: Press `F` to begin the next focus phase.
- **Customize**: Edit `settings.txt` to tweak phase durations or other settings.

## Further Work
Here are some ideas for enhancing the project:
- **GUI Settings Menu**: Add an in-app interface to adjust settings instead of editing the text file manually.
- **Notifications**: Implement visual or audio cues (e.g., a beep or flash) when a timer ends or a new phase begins.
- **Custom Phases**: Let users define their own phase sequences for greater flexibility.
- **Button-Based GUI**: Replace keyboard controls with clickable buttons to improve accessibility and ease of use.
- **Cross-Platform Support**: Extend compatibility to macOS and Linux by replacing Windows-specific API calls with portable alternatives.

## Contributing
I’d love feedback or contributions! Feel free to:
- Submit bug reports or feature ideas via [GitHub Issues](https://github.com/yourusername/ADHDPomodoro/issues).
- Fork the repo and send pull requests with improvements.\
## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---
Copyright (c) 2025 lm
