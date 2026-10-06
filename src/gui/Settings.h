#pragma once
#include <string>
#include <fstream>
#include <SFML/Graphics.hpp>

class Settings {
public:
    Settings();
    void loadSettings();
    void saveSettings();
    void drawMenuBar(sf::RenderWindow& window, sf::Font& font);
    bool handleMenuClick(const sf::Vector2i& mousePos);
    void handleSettingsInput(sf::Event& event);
    
    int getFocusDuration() const { return focusDuration; }
    int getRestDuration() const { return restDuration; }
    
    // New public methods for window management
    bool isWindowOpen() const { return isSettingsOpen; }
    bool pollSettingsEvent(sf::Event& event);
    void closeSettingsWindow();
    
private:
    int focusDuration;
    int restDuration;
    bool isSettingsOpen;
    std::string settingsFilePath;
    std::unique_ptr<sf::RenderWindow> settingsWindow;
    
    // Menu bar properties
    sf::RectangleShape menuBar;
    sf::Text settingsText;
    sf::FloatRect settingsTextBounds;
    
    // Settings window properties
    sf::RectangleShape focusDecreaseButton;
    sf::RectangleShape focusIncreaseButton;
    sf::RectangleShape restDecreaseButton;
    sf::RectangleShape restIncreaseButton;
    sf::RectangleShape saveButton;
    sf::RectangleShape closeButton;
    
    sf::Text focusLabel;
    sf::Text focusValue;
    sf::Text restLabel;
    sf::Text restValue;
    sf::Text saveButtonText;
    sf::Text closeButtonText;
    
    void drawSettingsWindow();
    bool isPointInRect(const sf::Vector2i& point, const sf::FloatRect& rect);
    void initializeUIElements(sf::Font& font);
    void updateButtonStates();
    void openSettingsWindow();
}; 