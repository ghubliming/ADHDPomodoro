#include "cli/StatsView.h"

#include <cstdio>

#include "cli/Render.h"
#include "core/TimeUtil.h"

namespace adhd::ui {

std::vector<std::string> statsReport(const Stats& st, bool color) {
    auto c = [color](const char* code) { return color ? std::string(code) : std::string(); };
    const std::string b = c(ansi::bold), d = c(ansi::dim), r = c(ansi::reset);
    std::vector<std::string> out;

    if (st.totalSessions == 0) {
        out.push_back("No sessions logged yet. Start one; even 5 minutes counts.");
        return out;
    }

    out.push_back(b + "Today      " + r + formatDuration(st.todayFocusSeconds) + " focused · " +
                  std::to_string(st.todaySessions) + (st.todaySessions == 1 ? " session" : " sessions"));
    out.push_back(b + "Streak     " + r + std::to_string(st.streakDays) + (st.streakDays == 1 ? " day" : " days"));

    std::string days, spark = sparkline(st.dailyFocusSeconds);
    for (std::size_t i = 0; i < st.dailyFocusSeconds.size(); ++i)
        days += std::string(weekdayShort(st.firstDay + static_cast<std::int64_t>(i))).substr(0, 1);
    int week = 0;
    for (int s : st.dailyFocusSeconds) week += s;
    out.push_back(b + "Last " + std::to_string(st.dailyFocusSeconds.size()) + " days " + r + c(ansi::cyan) + spark +
                  r + "  " + formatDuration(week));
    out.push_back("            " + d + days + r);
    out.push_back("");

    out.push_back(b + "All time   " + r + formatDuration(st.totalFocusSeconds) + " across " +
                  std::to_string(st.totalSessions) + (st.totalSessions == 1 ? " session" : " sessions"));
    out.push_back(b + "Ratings    " + r + c(ansi::green) + "good " + std::to_string(st.good) + r + " · " +
                  c(ansi::yellow) + "okay " + std::to_string(st.okay) + r + " · " + c(ansi::red) + "bad " +
                  std::to_string(st.bad) + r);
    out.push_back(b + "Sessions   " + r + "avg " + formatDuration(st.avgSessionSeconds) + " · longest " +
                  formatDuration(st.longestSessionSeconds) + " · hyperfocus ×" +
                  std::to_string(st.hyperfocusSessions));
    if (st.avgIdleSeconds >= 0)
        out.push_back(b + "Restart    " + r + "after a break you get going again in ~" +
                      formatDuration(st.avgIdleSeconds) + " on average");
    if (st.bestHour >= 0) {
        char buf[32];
        std::snprintf(buf, sizeof buf, "%02d:00–%02d:00", st.bestHour, (st.bestHour + 1) % 24);
        out.push_back(b + "Best hour  " + r + buf + d + "  (most focus starts here)" + r);
    }
    if (!st.topTasks.empty()) {
        out.push_back("");
        out.push_back(b + "Top tasks" + r);
        for (const auto& [task, secs] : st.topTasks)
            out.push_back("  " + padRight(truncate(task, 32), 34) + formatDuration(secs));
    }
    return out;
}

} // namespace adhd::ui
