#pragma once
#include <iosfwd>
#include <map>
#include <string>

namespace adhd {

// User settings, stored as a commented `key = value` text file.
// All durations are whole minutes.
struct Config {
    int focusMinutes = 15;      // current focus target (adapts when `adaptive` is on)
    int restMinutes = 5;        // minimum rest
    int tinyStartMinutes = 5;   // "just start" micro-session
    bool adaptive = true;       // let session ratings tune focusMinutes
    int adaptStepMinutes = 2;
    int minFocusMinutes = 5;    // adaptive bounds
    int maxFocusMinutes = 60;
    int restPercent = 20;       // proportional rest: long sessions earn longer breaks
    int maxRestMinutes = 30;
    int nudgeMinutes = 3;       // gentle reminder interval in ExtendedFocus / Idle (0 = off)
    int hyperfocusCheckMinutes = 30; // body check-in interval during Hyperfocus (0 = off)
    bool bell = true;           // terminal bell on phase ends and nudges
    bool notify = false;        // desktop notifications (notify-send / osascript)

    // Shell commands run in the background when a phase starts, keyed by
    // phase name ("focus", "rest", ...). "{phase}" is replaced by the phase name.
    std::map<std::string, std::string> hooks;

    void clamp();

    // Accepts the current format and the legacy v1 format ("<focus> <rest>").
    static Config parse(std::istream& in);
    void write(std::ostream& out) const;

    static Config load(const std::string& path, bool* existed = nullptr);
    bool save(const std::string& path) const;
};

} // namespace adhd
