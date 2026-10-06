#include <SFML/Graphics.hpp>
#include <windows.h>
#include <chrono>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "Settings.h"

// Timer states
enum class State {
    Focus,
    ExtendedFocus,
    Rest,
    Hyperfocus,
    IdleAfterRest
};

// Helper function declarations
bool isPointInRect(const sf::Vector2i& point, const sf::FloatRect& rect);
std::string formatTime(int seconds);
std::string getTimestamp();

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

// Check if a point is inside a rectangle
bool isPointInRect(const sf::Vector2i& point, const sf::FloatRect& rect) {
    return point.x >= rect.left && point.x <= rect.left + rect.width &&
           point.y >= rect.top && point.y <= rect.top + rect.height;
}

int main() {
    // Initialize window with fixed size and no resize
    sf::RenderWindow window(sf::VideoMode(300, 130), "ADHD Pomodoro Timer", sf::Style::Titlebar | sf::Style::Close);
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
    timerText.setPosition(10, 40);
    timerText.setFillColor(sf::Color::Black);

    // Set up buttons
    sf::RectangleShape startButton(sf::Vector2f(100, 30));
    startButton.setPosition(100, 70);
    startButton.setFillColor(sf::Color(200, 200, 200));

    sf::RectangleShape pauseButton(sf::Vector2f(100, 30));
    pauseButton.setPosition(100, 70);
    pauseButton.setFillColor(sf::Color(200, 200, 200));

    sf::RectangleShape finishButton(sf::Vector2f(100, 30));
    finishButton.setPosition(210, 70);
    finishButton.setFillColor(sf::Color(200, 200, 200));

    sf::Text startButtonText("Start", font, 16);
    startButtonText.setPosition(125, 75);
    startButtonText.setFillColor(sf::Color::Black);

    sf::Text pauseButtonText("Pause", font, 16);
    pauseButtonText.setPosition(125, 75);
    pauseButtonText.setFillColor(sf::Color::Black);

    sf::Text finishButtonText("Finish", font, 16);
    finishButtonText.setPosition(225, 75);
    finishButtonText.setFillColor(sf::Color::Black);

    // Initialize settings
    Settings settings;

    // Timer variables
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    TimePoint startTime = Clock::now();
    TimePoint pauseTime = Clock::now();
    State currentState = State::Focus;
    int elapsedSeconds = 0;
    int totalFocusTime = 0;      // For ExtendedFocus and Hyperfocus
    bool isTimerRunning = false;
    bool isPaused = false;
    std::ofstream logFile("log.txt", std::ios::app);

    while (window.isOpen()) {
        // Handle events
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);
                    
                    // Handle settings menu click
                    if (settings.handleMenuClick(mousePos)) {
                        continue;
                    }

                    // Handle start button click
                    if (!isTimerRunning && isPointInRect(mousePos, startButton.getGlobalBounds())) {
                        isTimerRunning = true;
                        isPaused = false;
                        startTime = Clock::now();
                        logFile << getTimestamp() << " - Session started\n";
                    }
                    // Handle pause button click
                    else if (isTimerRunning && !isPaused && isPointInRect(mousePos, pauseButton.getGlobalBounds())) {
                        isPaused = true;
                        pauseTime = Clock::now();
                        logFile << getTimestamp() << " - Session paused at " << formatTime(elapsedSeconds) << "\n";
                    }
                    // Handle resume button click
                    else if (isTimerRunning && isPaused && isPointInRect(mousePos, pauseButton.getGlobalBounds())) {
                        isPaused = false;
                        startTime += (Clock::now() - pauseTime);
                        logFile << getTimestamp() << " - Session resumed\n";
                    }
                    // Handle finish button click
                    else if (isTimerRunning && isPointInRect(mousePos, finishButton.getGlobalBounds())) {
                        isTimerRunning = false;
                        isPaused = false;
                        elapsedSeconds = 0;
                        logFile << getTimestamp() << " - Session finished at " << formatTime(elapsedSeconds) << "\n";
                    }
                }
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
                    int restDuration = totalFocusTime / 2; // Rest is half of hyperfocus duration
                    startTime = Clock::now();
                    logFile << getTimestamp() << " - Hyperfocus: " << totalFocusTime / 60 << " min\n";
                } else if (currentState == State::Rest && event.key.code == sf::Keyboard::F) {
                    currentState = State::Focus;
                    startTime = Clock::now();
                    logFile << getTimestamp() << " - Rest: " << settings.getRestDuration() << " min\n";
                } else if (currentState == State::IdleAfterRest && event.key.code == sf::Keyboard::F) {
                    currentState = State::Focus;
                    startTime = Clock::now();
                    logFile << getTimestamp() << " - Idle: " << elapsedSeconds / 60 << " min\n";
                }
            }
        }

        // Handle settings window events
        if (settings.isWindowOpen()) {
            while (settings.pollSettingsEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    settings.closeSettingsWindow();
                }
                else if (event.type == sf::Event::MouseButtonPressed) {
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);
                        settings.handleMenuClick(mousePos);
                    }
                }
            }
        }

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
                    }
                    timerText.setString("Focus: " + formatTime(remaining));
                    break;
                }
                case State::ExtendedFocus: {
                    totalFocusTime = settings.getFocusDuration() * 60 + elapsedSeconds;
                    timerText.setString("Extended Focus: " + formatTime(totalFocusTime) + "\nG/B/O/H");
                    break;
                }
                case State::Hyperfocus: {
                    totalFocusTime = settings.getFocusDuration() * 60 + elapsedSeconds;
                    timerText.setString("Hyperfocus: " + formatTime(totalFocusTime) + "\nPress R for Rest");
                    break;
                }
                case State::Rest: {
                    int remaining = settings.getRestDuration() * 60 - elapsedSeconds;
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
        } else if (isPaused) {
            timerText.setString("Paused: " + formatTime(elapsedSeconds) + "\nClick Pause to resume");
        } else {
            timerText.setString("Click Start to begin");
        }

        // Render
        window.clear(sf::Color::White);
        settings.drawMenuBar(window, font);
        window.draw(timerText);
        
        if (!isTimerRunning) {
            window.draw(startButton);
            window.draw(startButtonText);
        } else {
            window.draw(pauseButton);
            window.draw(isPaused ? pauseButtonText : pauseButtonText);
            window.draw(finishButton);
            window.draw(finishButtonText);
        }
        window.display();
    }

    logFile.close();
    return 0;
}