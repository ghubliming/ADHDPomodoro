#include <chrono>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <thread>
#include "CLISettings.h"

// Platform-specific includes for keyboard input
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#endif

/**
 * ADHD Pomodoro Timer - CLI Version
 * 
 * A command line implementation of the ADHD-focused Pomodoro timer.
 * Supports all the same states and features as the GUI version:
 * - Focus, ExtendedFocus, Rest, Hyperfocus, IdleAfterRest states
 * - Feedback system (Good/Bad/Okay/Hyperfocus ratings)
 * - Configurable durations via settings
 * - Session logging with timestamps
 * - Cross-platform keyboard input handling
 */

// Timer states
enum class State {
    Focus,
    ExtendedFocus,
    Rest,
    Hyperfocus,
    IdleAfterRest
};

// Forward declarations
std::string formatTime(int seconds);
std::string getTimestamp();
void clearScreen();
void displayTimer(State currentState, int elapsedSeconds, int totalFocusTime, 
                 bool isTimerRunning, bool isPaused, const CLISettings& settings);
#ifndef _WIN32
bool kbhit();
char getch();
#endif
void showHelp();
void showSettings(CLISettings& settings);

// Convert seconds to MM:SS format
std::string formatTime(int seconds) {
    int minutes = seconds / 60;
    int secs = seconds % 60;
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << secs;
    return oss.str();
}

// Get current timestamp for logging
std::string getTimestamp() {
    auto now = std::time(nullptr);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Clear screen
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Cross-platform keyboard input functions
#ifdef _WIN32
// On Windows, use the standard library functions directly
// _kbhit() and _getch() are already provided by conio.h
#else
static struct termios oldt, newt;
static bool termios_initialized = false;

void setup_terminal() {
    if (!termios_initialized) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        termios_initialized = true;
    }
}

bool kbhit() {
    setup_terminal();
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    fd_set rdfs;
    FD_ZERO(&rdfs);
    FD_SET(STDIN_FILENO, &rdfs);
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    int result = select(STDIN_FILENO + 1, &rdfs, NULL, NULL, &timeout);
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return result > 0;
}

char getch() {
    setup_terminal();
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    int ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif

void showHelp() {
    std::cout << "\n=== ADHD Pomodoro Timer - CLI Version ===\n\n";
    std::cout << "Controls:\n";
    std::cout << "  s/S - Start timer\n";
    std::cout << "  p/P - Pause/Resume timer\n";
    std::cout << "  f/F - Finish current session\n";
    std::cout << "  c/C - Show/Edit settings\n";
    std::cout << "  h/H - Show this help\n";
    std::cout << "  q/Q - Quit\n\n";
    std::cout << "During Extended Focus:\n";
    std::cout << "  g/G - Good session (go to rest)\n";
    std::cout << "  b/B - Bad session (go to rest)\n";
    std::cout << "  o/O - Okay session (go to rest)\n";
    std::cout << "  h/H - Enter Hyperfocus mode\n\n";
    std::cout << "During Hyperfocus:\n";
    std::cout << "  r/R - Go to rest\n\n";
    std::cout << "During Rest/Idle:\n";
    std::cout << "  f/F - Start new focus session\n\n";
    std::cout << "Press any key to continue...";
    getch();
}

void showSettings(CLISettings& settings) {
    char choice;
    do {
        clearScreen();
        std::cout << "\n=== Settings ===\n\n";
        std::cout << "Current Settings:\n";
        std::cout << "  Focus Duration: " << settings.getFocusDuration() << " minutes\n";
        std::cout << "  Rest Duration: " << settings.getRestDuration() << " minutes\n\n";
        std::cout << "Options:\n";
        std::cout << "  1 - Increase Focus Duration\n";
        std::cout << "  2 - Decrease Focus Duration\n";
        std::cout << "  3 - Increase Rest Duration\n";
        std::cout << "  4 - Decrease Rest Duration\n";
        std::cout << "  s - Save and return\n";
        std::cout << "  q - Return without saving\n\n";
        std::cout << "Choice: ";
        
        choice = getch();
        std::cout << choice << std::endl;
        
        switch (choice) {
            case '1':
                settings.increaseFocusDuration();
                break;
            case '2':
                settings.decreaseFocusDuration();
                break;
            case '3':
                settings.increaseRestDuration();
                break;
            case '4':
                settings.decreaseRestDuration();
                break;
            case 's':
            case 'S':
                settings.saveSettings();
                return;
            case 'q':
            case 'Q':
                return;
        }
    } while (true);
}

void displayTimer(State currentState, int elapsedSeconds, int totalFocusTime, 
                 bool isTimerRunning, bool isPaused, const CLISettings& settings) {
    clearScreen();
    std::cout << "\n=== ADHD Pomodoro Timer - CLI Version ===\n\n";
    
    if (!isTimerRunning) {
        std::cout << "Status: Ready to start\n";
        std::cout << "Press 's' to start, 'h' for help, 'q' to quit\n";
    } else if (isPaused) {
        std::cout << "Status: PAUSED - " << formatTime(elapsedSeconds) << "\n";
        std::cout << "Press 'p' to resume, 'f' to finish\n";
    } else {
        switch (currentState) {
            case State::Focus: {
                int remaining = settings.getFocusDuration() * 60 - elapsedSeconds;
                std::cout << "Status: FOCUS - " << formatTime(remaining > 0 ? remaining : 0) << "\n";
                if (remaining > 0) {
                    std::cout << "Stay focused! Press 'p' to pause, 'f' to finish\n";
                } else {
                    std::cout << "Focus time complete! Entering Extended Focus...\n";
                }
                break;
            }
            case State::ExtendedFocus:
                std::cout << "Status: EXTENDED FOCUS - " << formatTime(totalFocusTime) << "\n";
                std::cout << "How was your session? g=Good, b=Bad, o=Okay, h=Hyperfocus\n";
                break;
            case State::Hyperfocus:
                std::cout << "Status: HYPERFOCUS - " << formatTime(totalFocusTime) << "\n";
                std::cout << "Deep focus mode! Press 'r' when ready for rest\n";
                break;
            case State::Rest: {
                int remaining = settings.getRestDuration() * 60 - elapsedSeconds;
                std::cout << "Status: REST - " << formatTime(remaining > 0 ? remaining : 0) << "\n";
                if (remaining > 0) {
                    std::cout << "Take a break! Press 'f' to start focus early\n";
                } else {
                    std::cout << "Rest complete! Press 'f' to focus or wait...\n";
                }
                break;
            }
            case State::IdleAfterRest:
                std::cout << "Status: IDLE - " << formatTime(elapsedSeconds) << "\n";
                std::cout << "Press 'f' to start next focus session\n";
                break;
        }
    }
    
    std::cout << "\nSettings: Focus=" << settings.getFocusDuration() 
              << "min, Rest=" << settings.getRestDuration() << "min\n";
    std::cout << "Press 'h' for help, 'c' for settings, 'q' to quit\n";
}

int main() {
    std::cout << "Starting ADHD Pomodoro Timer - CLI Version...\n";
    
    // Initialize settings
    CLISettings settings;
    
    // Timer variables
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    TimePoint startTime = Clock::now();
    TimePoint pauseTime = Clock::now();
    State currentState = State::Focus;
    int elapsedSeconds = 0;
    int totalFocusTime = 0;
    bool isTimerRunning = false;
    bool isPaused = false;
    std::ofstream logFile("log.txt", std::ios::app);
    
    // Show initial help
    showHelp();
    
    // Main loop
    while (true) {
        // Update timer only if running and not paused
        if (isTimerRunning && !isPaused) {
            auto now = Clock::now();
            elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
            
            // State machine
            switch (currentState) {
                case State::Focus: {
                    int remaining = settings.getFocusDuration() * 60 - elapsedSeconds;
                    if (remaining <= 0) {
                        currentState = State::ExtendedFocus;
                        totalFocusTime = settings.getFocusDuration() * 60;
                        startTime = Clock::now();
                        elapsedSeconds = 0;
                    }
                    break;
                }
                case State::ExtendedFocus: {
                    totalFocusTime = settings.getFocusDuration() * 60 + elapsedSeconds;
                    break;
                }
                case State::Hyperfocus: {
                    totalFocusTime = settings.getFocusDuration() * 60 + elapsedSeconds;
                    break;
                }
                case State::Rest: {
                    int remaining = settings.getRestDuration() * 60 - elapsedSeconds;
                    if (remaining <= 0) {
                        currentState = State::IdleAfterRest;
                        startTime = Clock::now();
                        elapsedSeconds = 0;
                    }
                    break;
                }
                case State::IdleAfterRest:
                    // Just count up
                    break;
            }
        }
        
        // Display current status
        displayTimer(currentState, elapsedSeconds, totalFocusTime, isTimerRunning, isPaused, settings);
        
        // Handle keyboard input (non-blocking)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
#ifdef _WIN32
        if (_kbhit()) {
            char key = static_cast<char>(_getch());
#else
        if (kbhit()) {
            char key = getch();
#endif
            key = tolower(key);
            
            switch (key) {
                case 'q':
                    logFile << getTimestamp() << " - Application quit\n";
                    logFile.close();
                    std::cout << "\nGoodbye!\n";
                    return 0;
                    
                case 'h':
                    showHelp();
                    break;
                    
                case 'c':
                    showSettings(settings);
                    break;
                    
                case 's':
                    if (!isTimerRunning) {
                        isTimerRunning = true;
                        isPaused = false;
                        currentState = State::Focus;
                        startTime = Clock::now();
                        elapsedSeconds = 0;
                        totalFocusTime = 0;
                        logFile << getTimestamp() << " - Session started\n";
                    }
                    break;
                    
                case 'p':
                    if (isTimerRunning) {
                        if (isPaused) {
                            isPaused = false;
                            startTime += (Clock::now() - pauseTime);
                            logFile << getTimestamp() << " - Session resumed\n";
                        } else {
                            isPaused = true;
                            pauseTime = Clock::now();
                            logFile << getTimestamp() << " - Session paused at " << formatTime(elapsedSeconds) << "\n";
                        }
                    }
                    break;
                    
                case 'f':
                    if (isTimerRunning) {
                        if (currentState == State::Rest || currentState == State::IdleAfterRest) {
                            // Start new focus session
                            currentState = State::Focus;
                            startTime = Clock::now();
                            elapsedSeconds = 0;
                            totalFocusTime = 0;
                            isPaused = false;
                            logFile << getTimestamp() << " - New focus session started\n";
                        } else {
                            // Finish current session
                            isTimerRunning = false;
                            isPaused = false;
                            logFile << getTimestamp() << " - Session finished at " << formatTime(elapsedSeconds) << "\n";
                        }
                    }
                    break;
                    
                // Extended Focus feedback
                case 'g':
                    if (currentState == State::ExtendedFocus) {
                        currentState = State::Rest;
                        startTime = Clock::now();
                        elapsedSeconds = 0;
                        logFile << getTimestamp() << " - Focus: " << totalFocusTime / 60 << " min (Good)\n";
                    }
                    break;
                    
                case 'b':
                    if (currentState == State::ExtendedFocus) {
                        currentState = State::Rest;
                        startTime = Clock::now();
                        elapsedSeconds = 0;
                        logFile << getTimestamp() << " - Focus: " << totalFocusTime / 60 << " min (Bad)\n";
                    }
                    break;
                    
                case 'o':
                    if (currentState == State::ExtendedFocus) {
                        currentState = State::Rest;
                        startTime = Clock::now();
                        elapsedSeconds = 0;
                        logFile << getTimestamp() << " - Focus: " << totalFocusTime / 60 << " min (Okay)\n";
                    }
                    break;
                    
                // Hyperfocus
                case 'r':
                    if (currentState == State::Hyperfocus) {
                        currentState = State::Rest;
                        startTime = Clock::now();
                        elapsedSeconds = 0;
                        logFile << getTimestamp() << " - Hyperfocus: " << totalFocusTime / 60 << " min\n";
                    }
                    break;
            }
            
            // Handle hyperfocus transition from extended focus
            if (key == 'h' && currentState == State::ExtendedFocus) {
                currentState = State::Hyperfocus;
                logFile << getTimestamp() << " - Entered hyperfocus mode\n";
            }
        }
    }
    
    logFile.close();
    return 0;
}