#pragma once
#include <string>

namespace adhd {

// Where settings, the session journal and parked thoughts live.
struct Paths {
    std::string dir;
    std::string config;   // settings.conf
    std::string journal;  // sessions.csv
    std::string parking;  // parked.md

    // Precedence: explicit dir, $ADHD_POMODORO_HOME, %APPDATA%\ADHDPomodoro (Windows),
    // $XDG_DATA_HOME/adhd-pomodoro, ~/.local/share/adhd-pomodoro, then ".".
    static Paths resolve(const std::string& explicitDir = "");
    static Paths in(const std::string& dir);
    bool ensureDir() const;
};

} // namespace adhd
