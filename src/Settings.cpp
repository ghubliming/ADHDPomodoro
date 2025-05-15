#include "Settings.h"
#include <iostream>
#include <filesystem>

Settings::Settings() : focusDuration(10), restDuration(5), isSettingsOpen(false) {
    settingsFilePath = "settings.txt";
    loadSettings();
}

void Settings::initializeUIElements(sf::Font& font) {
    // Initialize menu bar
    menuBar.setSize(sf::Vector2f(300, 30));
    menuBar.setFillColor(sf::Color(240, 240, 240));
    
    settingsText.setFont(font);
    settingsText.setString("Settings");
    settingsText.setCharacterSize(16);
    settingsText.setFillColor(sf::Color(50, 50, 50));
    settingsText.setPosition(10, 5);
    
    // Create a clickable area for the settings text
    settingsTextBounds = settingsText.getGlobalBounds();
    // Add some padding to make it easier to click
    settingsTextBounds.width += 20;
    settingsTextBounds.height += 10;
    settingsTextBounds.left -= 5;
    settingsTextBounds.top -= 5;

    // Initialize buttons
    float buttonSize = 25.f;
    float buttonSpacing = 5.f;
    
    // Focus duration controls
    focusDecreaseButton.setSize(sf::Vector2f(buttonSize, buttonSize));
    focusDecreaseButton.setPosition(180, 45);
    focusDecreaseButton.setFillColor(sf::Color(200, 200, 200));
    
    focusIncreaseButton.setSize(sf::Vector2f(buttonSize, buttonSize));
    focusIncreaseButton.setPosition(210, 45);
    focusIncreaseButton.setFillColor(sf::Color(200, 200, 200));
    
    // Rest duration controls
    restDecreaseButton.setSize(sf::Vector2f(buttonSize, buttonSize));
    restDecreaseButton.setPosition(180, 75);
    restDecreaseButton.setFillColor(sf::Color(200, 200, 200));
    
    restIncreaseButton.setSize(sf::Vector2f(buttonSize, buttonSize));
    restIncreaseButton.setPosition(210, 75);
    restIncreaseButton.setFillColor(sf::Color(200, 200, 200));
    
    // Action buttons
    saveButton.setSize(sf::Vector2f(80, 30));
    saveButton.setPosition(20, 150);
    saveButton.setFillColor(sf::Color(200, 200, 200));
    
    closeButton.setSize(sf::Vector2f(80, 30));
    closeButton.setPosition(150, 150);
    closeButton.setFillColor(sf::Color(200, 200, 200));

    // Initialize text elements with darker colors
    focusLabel.setFont(font);
    focusLabel.setString("Focus Duration (minutes):");
    focusLabel.setCharacterSize(14);
    focusLabel.setPosition(20, 50);
    focusLabel.setFillColor(sf::Color(50, 50, 50));
    
    focusValue.setFont(font);
    focusValue.setCharacterSize(14);
    focusValue.setPosition(185, 50);
    focusValue.setFillColor(sf::Color(50, 50, 50));
    
    restLabel.setFont(font);
    restLabel.setString("Rest Duration (minutes):");
    restLabel.setCharacterSize(14);
    restLabel.setPosition(20, 80);
    restLabel.setFillColor(sf::Color(50, 50, 50));
    
    restValue.setFont(font);
    restValue.setCharacterSize(14);
    restValue.setPosition(185, 80);
    restValue.setFillColor(sf::Color(50, 50, 50));
    
    saveButtonText.setFont(font);
    saveButtonText.setString("Save");
    saveButtonText.setCharacterSize(14);
    saveButtonText.setPosition(45, 155);
    saveButtonText.setFillColor(sf::Color(50, 50, 50));
    
    closeButtonText.setFont(font);
    closeButtonText.setString("Close");
    closeButtonText.setCharacterSize(14);
    closeButtonText.setPosition(175, 155);
    closeButtonText.setFillColor(sf::Color(50, 50, 50));
}

void Settings::openSettingsWindow() {
    if (!settingsWindow) {
        settingsWindow = std::make_unique<sf::RenderWindow>(
            sf::VideoMode(300, 200),
            "Pomodoro Settings",
            sf::Style::Titlebar | sf::Style::Close
        );
        isSettingsOpen = true;
    }
}

void Settings::closeSettingsWindow() {
    if (settingsWindow) {
        settingsWindow->close();
        settingsWindow.reset();
        isSettingsOpen = false;
    }
}

void Settings::loadSettings() {
    std::ifstream settingsFile(settingsFilePath);
    if (settingsFile.is_open()) {
        settingsFile >> focusDuration >> restDuration;
        settingsFile.close();
    } else {
        saveSettings();
    }
}

void Settings::saveSettings() {
    std::ofstream settingsFile(settingsFilePath);
    if (settingsFile.is_open()) {
        settingsFile << focusDuration << " " << restDuration;
        settingsFile.close();
    }
}

void Settings::drawMenuBar(sf::RenderWindow& window, sf::Font& font) {
    if (settingsText.getFont() == nullptr) {
        initializeUIElements(font);
    }

    // Update settings text bounds
    settingsTextBounds = settingsText.getGlobalBounds();
    settingsTextBounds.width += 20;
    settingsTextBounds.height += 10;
    settingsTextBounds.left -= 5;
    settingsTextBounds.top -= 5;

    // Draw menu bar with a slight highlight when mouse is over settings
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    if (mousePos.y < 30 && isPointInRect(mousePos, settingsTextBounds)) {
        menuBar.setFillColor(sf::Color(220, 220, 220));
    } else {
        menuBar.setFillColor(sf::Color(240, 240, 240));
    }

    window.draw(menuBar);
    window.draw(settingsText);

    if (isSettingsOpen && settingsWindow) {
        drawSettingsWindow();
    }
}

void Settings::drawSettingsWindow() {
    if (!settingsWindow) return;

    settingsWindow->clear(sf::Color(240, 240, 240));

    // Update values
    focusValue.setString(std::to_string(focusDuration));
    restValue.setString(std::to_string(restDuration));
    
    // Draw buttons
    settingsWindow->draw(focusDecreaseButton);
    settingsWindow->draw(focusIncreaseButton);
    settingsWindow->draw(restDecreaseButton);
    settingsWindow->draw(restIncreaseButton);
    settingsWindow->draw(saveButton);
    settingsWindow->draw(closeButton);
    
    // Draw text elements
    settingsWindow->draw(focusLabel);
    settingsWindow->draw(focusValue);
    settingsWindow->draw(restLabel);
    settingsWindow->draw(restValue);
    settingsWindow->draw(saveButtonText);
    settingsWindow->draw(closeButtonText);

    settingsWindow->display();
}

bool Settings::handleMenuClick(const sf::Vector2i& mousePos) {
    // Check if settings menu item was clicked
    if (mousePos.y < 30) {
        // Debug output
        std::cout << "Mouse click at: (" << mousePos.x << ", " << mousePos.y << ")" << std::endl;
        std::cout << "Settings bounds: (" << settingsTextBounds.left << ", " << settingsTextBounds.top 
                  << ", " << settingsTextBounds.width << ", " << settingsTextBounds.height << ")" << std::endl;
        
        if (isPointInRect(mousePos, settingsTextBounds)) {
            std::cout << "Settings clicked!" << std::endl;
            if (isSettingsOpen) {
                closeSettingsWindow();
            } else {
                openSettingsWindow();
            }
            return true;
        }
    }

    if (isSettingsOpen && settingsWindow) {
        // Convert mouse position to settings window coordinates
        sf::Vector2i settingsMousePos = mousePos - sf::Vector2i(0, 30);

        // Check save button
        if (isPointInRect(settingsMousePos, saveButton.getGlobalBounds())) {
            saveSettings();
            return true;
        }
        // Check close button
        if (isPointInRect(settingsMousePos, closeButton.getGlobalBounds())) {
            closeSettingsWindow();
            return true;
        }
        // Check focus duration buttons
        if (isPointInRect(settingsMousePos, focusDecreaseButton.getGlobalBounds())) {
            if (focusDuration > 1) focusDuration--;
            updateButtonStates();
            return true;
        }
        if (isPointInRect(settingsMousePos, focusIncreaseButton.getGlobalBounds())) {
            if (focusDuration < 60) focusDuration++;
            updateButtonStates();
            return true;
        }
        // Check rest duration buttons
        if (isPointInRect(settingsMousePos, restDecreaseButton.getGlobalBounds())) {
            if (restDuration > 1) restDuration--;
            updateButtonStates();
            return true;
        }
        if (isPointInRect(settingsMousePos, restIncreaseButton.getGlobalBounds())) {
            if (restDuration < 30) restDuration++;
            updateButtonStates();
            return true;
        }
    }

    return false;
}

void Settings::handleSettingsInput(sf::Event& event) {
    if (isSettingsOpen && settingsWindow) {
        sf::Event settingsEvent;
        while (settingsWindow->pollEvent(settingsEvent)) {
            if (settingsEvent.type == sf::Event::Closed) {
                closeSettingsWindow();
            }
            else if (settingsEvent.type == sf::Event::MouseButtonPressed) {
                if (settingsEvent.mouseButton.button == sf::Mouse::Left) {
                    handleMenuClick(sf::Vector2i(settingsEvent.mouseButton.x, settingsEvent.mouseButton.y));
                }
            }
        }
    }
}

bool Settings::isPointInRect(const sf::Vector2i& point, const sf::FloatRect& rect) {
    return point.x >= rect.left && point.x <= rect.left + rect.width &&
           point.y >= rect.top && point.y <= rect.top + rect.height;
}

void Settings::updateButtonStates() {
    focusDecreaseButton.setFillColor(focusDuration > 1 ? sf::Color(200, 200, 200) : sf::Color(150, 150, 150));
    focusIncreaseButton.setFillColor(focusDuration < 60 ? sf::Color(200, 200, 200) : sf::Color(150, 150, 150));
    restDecreaseButton.setFillColor(restDuration > 1 ? sf::Color(200, 200, 200) : sf::Color(150, 150, 150));
    restIncreaseButton.setFillColor(restDuration < 30 ? sf::Color(200, 200, 200) : sf::Color(150, 150, 150));
}

bool Settings::pollSettingsEvent(sf::Event& event) {
    if (isSettingsOpen && settingsWindow) {
        return settingsWindow->pollEvent(event);
    }
    return false; 
} 