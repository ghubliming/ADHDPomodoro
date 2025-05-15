#include <SFML/Graphics.hpp>
#include <windows.h>
#include <chrono>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#include <iostream>

// Timer states
enum class State {
    Focus,
    ExtendedFocus,
    Rest,
    Hyperfocus,
    IdleAfterRest
};

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

int main() {
    // Initialize window
    sf::RenderWindow window(sf::VideoMode(300, 100), "ADHD Pomodoro Timer");
    HWND hwnd = window.getSystemHandle();
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); // Always on top

    // Load font
    sf::Font font;
    if (!font.loadFromFile("D:/CodeSSD/CLion/ADHDPomodoro/dll/arial.ttf")) {
        std::cerr << "Error: Failed to load font" << std::endl;
        return 1; // Exit if font fails to load
    } else {
        std::cout << "Font loaded successfully" << std::endl;
    }

    // Set up text display
    sf::Text timerText("", font, 20);
    timerText.setPosition(10, 10);
    timerText.setFillColor(sf::Color::Black);

    // Timer variables
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    TimePoint startTime = Clock::now();
    State currentState = State::Focus;
    int focusDuration = 10 * 60; // 10 minutes
    int restDuration = 5 * 60;   // 5 minutes
    int elapsedSeconds = 0;
    int totalFocusTime = 0;      // For ExtendedFocus and Hyperfocus
    std::ofstream logFile("log.txt", std::ios::app);

    while (window.isOpen()) {
        // Event handling
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed) {
                if (currentState == State::ExtendedFocus) {
                    if (event.key.code == sf::Keyboard::G) { // Good
                        currentState = State::Rest;
                        startTime = Clock::now();
                        logFile << getTimestamp() << " - Focus: " << totalFocusTime / 60 << " min (Good)\n";
                    } else if (event.key.code == sf::Keyboard::B) { // Bad
                        currentState = State::Rest;
                        startTime = Clock::now();
                        logFile << getTimestamp() << " - Focus: " << totalFocusTime / 60 << " min (Bad)\n";
                    } else if (event.key.code == sf::Keyboard::O) { // Okay
                        currentState = State::Rest;
                        startTime = Clock::now();
                        logFile << getTimestamp() << " - Focus: " << totalFocusTime / 60 << " min (Okay)\n";
                    } else if (event.key.code == sf::Keyboard::H) { // Hyperfocus
                        currentState = State::Hyperfocus;
                    }
                } else if (currentState == State::Hyperfocus && event.key.code == sf::Keyboard::R) {
                    currentState = State::Rest;
                    restDuration = totalFocusTime / 2; // Rest is half of hyperfocus duration
                    startTime = Clock::now();
                    logFile << getTimestamp() << " - Hyperfocus: " << totalFocusTime / 60 << " min\n";
                } else if (currentState == State::Rest && event.key.code == sf::Keyboard::F) {
                    currentState = State::Focus;
                    startTime = Clock::now();
                    logFile << getTimestamp() << " - Rest: " << restDuration / 60 << " min\n";
                } else if (currentState == State::IdleAfterRest && event.key.code == sf::Keyboard::F) {
                    currentState = State::Focus;
                    startTime = Clock::now();
                    logFile << getTimestamp() << " - Idle: " << elapsedSeconds / 60 << " min\n";
                }
            }
        }

        // Update timer
        auto now = Clock::now();
        elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();

        // State machine
        switch (currentState) {
            case State::Focus: {
                int remaining = focusDuration - elapsedSeconds;
                if (remaining <= 0) {
                    currentState = State::ExtendedFocus;
                    totalFocusTime = focusDuration;
                }
                timerText.setString("Focus: " + formatTime(remaining));
                break;
            }
            case State::ExtendedFocus: {
                totalFocusTime = focusDuration + elapsedSeconds;
                timerText.setString("Extended Focus: " + formatTime(totalFocusTime) + "\nG/B/O/H");
                break;
            }
            case State::Hyperfocus: {
                totalFocusTime = focusDuration + elapsedSeconds;
                timerText.setString("Hyperfocus: " + formatTime(totalFocusTime) + "\nPress R for Rest");
                break;
            }
            case State::Rest: {
                int remaining = restDuration - elapsedSeconds;
                if (remaining <= 0) {
                    currentState = State::IdleAfterRest;
                    startTime = Clock::now();
                }
                timerText.setString("Rest: " + formatTime(remaining) + "\nPress F to Focus");
                break;
            }
            case State::IdleAfterRest: {
                timerText.setString("Idle: " + formatTime(elapsedSeconds) + "\nPress F to Focus");
                break;
            }
        }

        // Render
        window.clear(sf::Color::White);
        window.draw(timerText);
        window.display();
    }

    logFile.close();
    return 0;
}