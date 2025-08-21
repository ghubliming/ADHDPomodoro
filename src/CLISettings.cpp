#include "CLISettings.h"
#include <iostream>
#include <fstream>

CLISettings::CLISettings() : focusDuration(10), restDuration(5) {
    settingsFilePath = "settings.txt";
    loadSettings();
}

void CLISettings::loadSettings() {
    std::ifstream settingsFile(settingsFilePath);
    if (settingsFile.is_open()) {
        settingsFile >> focusDuration >> restDuration;
        settingsFile.close();
        std::cout << "Settings loaded: Focus=" << focusDuration << "min, Rest=" << restDuration << "min" << std::endl;
    } else {
        std::cout << "No settings file found. Using defaults: Focus=" << focusDuration << "min, Rest=" << restDuration << "min" << std::endl;
        saveSettings();
    }
}

void CLISettings::saveSettings() {
    std::ofstream settingsFile(settingsFilePath);
    if (settingsFile.is_open()) {
        settingsFile << focusDuration << " " << restDuration;
        settingsFile.close();
        std::cout << "Settings saved: Focus=" << focusDuration << "min, Rest=" << restDuration << "min" << std::endl;
    } else {
        std::cerr << "Error: Could not save settings file" << std::endl;
    }
}