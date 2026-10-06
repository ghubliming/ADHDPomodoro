#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "cli/Terminal.h"
#include "core/Clock.h"
#include "core/Config.h"
#include "core/Engine.h"
#include "core/Journal.h"
#include "core/ParkingLot.h"
#include "core/Paths.h"
#include "core/Stats.h"

namespace adhd::ui {

// The interactive full-screen timer.
class App {
public:
    App(Paths paths, Config config);
    void setTask(std::string task) { engine_.setTask(std::move(task)); }
    int run();

private:
    enum class Mode { Normal, Input, Overlay, Settings, ConfirmQuit };
    enum class InputPurpose { Park, Task };

    struct SettingRow {
        const char* label;
        int* value;  // numeric setting, or
        bool* flag;  // on/off setting
        int lo, hi;
        const char* unit;
    };

    // Engine events -> side effects (journal, bell, hooks, notifications).
    void onEvent(const Engine::Event& e);

    void handleKey(const Key& k);
    void handleNormalKey(const Key& k);
    void handleInputKey(const Key& k);
    void handleSettingsKey(const Key& k);

    void start(bool tiny);
    void rate(Rating r);
    void beginInput(InputPurpose purpose);
    void showOverlay(std::string title, std::vector<std::string> lines);
    void showHelp();
    void showStats();
    void showParked();

    void render();
    std::vector<std::string> frame(int width);
    void timerBody(std::vector<std::string>& out, int width);
    void settingsBody(std::vector<std::string>& out, int width);
    std::string phaseMessage() const;
    std::string keyHints() const;
    std::vector<SettingRow> settingRows();

    void toast(std::string message);
    void bell();
    void notify(const char* message) const;
    void runHook(Phase phase) const;
    void refreshStats();
    int todayFocusSeconds() const;
    std::int64_t nowMs() const { return clock_.monotonicMs(); }

    Paths paths_;
    Config cfg_;
    SystemClock clock_;
    Engine engine_;
    Journal journal_;
    ParkingLot parking_;
    Stats stats_;
    int parkedCount_ = 0;

    Terminal* term_ = nullptr;
    Mode mode_ = Mode::Normal;
    InputPurpose inputPurpose_ = InputPurpose::Park;
    std::string input_;
    std::string overlayTitle_;
    std::vector<std::string> overlayLines_;
    int settingsIndex_ = 0;
    std::string toast_;
    std::int64_t toastUntilMs_ = 0;
    std::string adjustNote_;
    std::string lastFrame_;
    std::string lastTitle_;
    int lastWidth_ = 0, lastHeight_ = 0;
    bool quit_ = false;
};

} // namespace adhd::ui
