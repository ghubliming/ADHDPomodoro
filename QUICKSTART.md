#!/bin/bash

# ADHD Pomodoro CLI - Quick Start Guide and Examples

echo "=== ADHD Pomodoro CLI - Quick Start Guide ==="
echo ""

echo "1. Building the CLI version:"
echo "   mkdir build && cd build"
echo "   cmake .. -DBUILD_CLI_ONLY=ON"
echo "   make"
echo ""

echo "2. Running the application:"
echo "   ./ADHDPomodoro-CLI"
echo ""

echo "3. Basic Usage Flow:"
echo "   - Press 's' to start a focus session"
echo "   - Focus timer counts down from your set duration (default 10 min)"
echo "   - After focus time, you enter Extended Focus mode"
echo "   - Rate your session: 'g' (good), 'b' (bad), 'o' (okay), or 'h' (hyperfocus)"
echo "   - Take a rest break, then press 'f' to start next focus session"
echo ""

echo "4. Key Features:"
echo "   - Cross-platform (Windows & Linux)"
echo "   - Same timer logic as GUI version"
echo "   - All ADHD-focused states: Focus → Extended → Rest → Idle"
echo "   - Hyperfocus mode for deep work sessions"
echo "   - Settings adjustment via 'c' key"
echo "   - Session logging to log.txt"
echo "   - Persistent settings in settings.txt"
echo ""

echo "5. Controls Quick Reference:"
echo "   s/S - Start timer"
echo "   p/P - Pause/Resume"
echo "   f/F - Finish session (or start new focus during rest)"
echo "   c/C - Settings menu"
echo "   h/H - Help (or enter hyperfocus from extended focus)"
echo "   q/Q - Quit"
echo ""
echo "   During Extended Focus: g/b/o/h for feedback"
echo "   During Hyperfocus: r to go to rest"
echo "   During Rest/Idle: f to start new focus"
echo ""

echo "6. Example Session Flow:"
echo "   [Start] → [Focus 10min] → [Extended Focus: rate it] → [Rest 5min] → [Idle: start next]"
echo ""

echo "Build complete! Ready to use:"
echo "cd /home/runner/work/ADHDPomodoro/ADHDPomodoro/build && ./ADHDPomodoro-CLI"