#pragma once
#include <cstdint>
#include <ctime>
#include <string>
#include <utility>
#include <vector>

#include "core/Engine.h"

namespace adhd {

// A focus session and the rest/idle that followed it.
struct SessionSummary {
    std::int64_t id = 0;
    std::time_t start = 0;
    int focusSeconds = 0; // focus + extended + hyperfocus
    int restSeconds = 0;
    int idleSeconds = 0;  // time it took to come back after the break
    bool hyperfocus = false;
    bool sawIdle = false;
    Rating rating = Rating::None;
    std::string task;
};

std::vector<SessionSummary> summarizeSessions(const std::vector<PhaseRecord>& records);

struct Stats {
    int todayFocusSeconds = 0;
    int todaySessions = 0;
    int streakDays = 0;                 // consecutive days with focus, ending today (or yesterday)
    std::int64_t firstDay = 0;          // day number of dailyFocusSeconds[0]
    std::vector<int> dailyFocusSeconds; // oldest .. today

    int totalSessions = 0;
    int totalFocusSeconds = 0;
    int good = 0, okay = 0, bad = 0;
    int hyperfocusSessions = 0;
    int avgSessionSeconds = 0;
    int longestSessionSeconds = 0;
    int avgIdleSeconds = -1; // -1: never went idle after a break
    int bestHour = -1;       // hour of day (0-23) with the most focus, -1 if none
    std::vector<std::pair<std::string, int>> topTasks; // task -> focus seconds
};

Stats computeStats(const std::vector<PhaseRecord>& records, std::int64_t today, int days = 7);

// "▁▃█▅" style mini chart, one cell per value.
std::string sparkline(const std::vector<int>& values);

} // namespace adhd
