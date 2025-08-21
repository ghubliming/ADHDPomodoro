#pragma once
#include <string>
#include <fstream>

class CLISettings {
public:
    CLISettings();
    void loadSettings();
    void saveSettings();
    
    int getFocusDuration() const { return focusDuration; }
    int getRestDuration() const { return restDuration; }
    
    void increaseFocusDuration() { if (focusDuration < 120) focusDuration++; }
    void decreaseFocusDuration() { if (focusDuration > 1) focusDuration--; }
    void increaseRestDuration() { if (restDuration < 60) restDuration++; }
    void decreaseRestDuration() { if (restDuration > 1) restDuration--; }
    
private:
    int focusDuration;  // in minutes
    int restDuration;   // in minutes
    std::string settingsFilePath;
};