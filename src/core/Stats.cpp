#include "core/Stats.h"

#include <algorithm>
#include <array>
#include <map>

#include "core/TimeUtil.h"

namespace adhd {

std::vector<SessionSummary> summarizeSessions(const std::vector<PhaseRecord>& records) {
    std::map<std::int64_t, SessionSummary> byId;
    for (const auto& r : records) {
        auto [it, inserted] = byId.try_emplace(r.sessionId);
        SessionSummary& s = it->second;
        if (inserted) {
            s.id = r.sessionId;
            s.start = r.startedAt;
        }
        s.start = std::min(s.start, r.startedAt);
        if (isFocusPhase(r.phase)) s.focusSeconds += r.seconds;
        if (r.phase == Phase::Hyperfocus) s.hyperfocus = true;
        if (r.phase == Phase::Rest) s.restSeconds += r.seconds;
        if (r.phase == Phase::Idle) {
            s.idleSeconds += r.seconds;
            s.sawIdle = true;
        }
        if (r.rating != Rating::None) s.rating = r.rating;
        if (!r.task.empty()) s.task = r.task;
    }
    std::vector<SessionSummary> out;
    out.reserve(byId.size());
    for (auto& [id, s] : byId) {
        // Unrated blips under a minute are accidental starts, not sessions.
        if (s.focusSeconds >= 60 || (s.focusSeconds > 0 && s.rating != Rating::None)) out.push_back(std::move(s));
    }
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) { return a.start < b.start; });
    return out;
}

Stats computeStats(const std::vector<PhaseRecord>& records, std::int64_t today, int days) {
    Stats st;
    days = std::max(days, 1);
    st.firstDay = today - days + 1;
    st.dailyFocusSeconds.assign(static_cast<std::size_t>(days), 0);

    std::map<std::int64_t, int> focusByDay;
    std::array<int, 24> focusByHour{};
    std::map<std::string, int> focusByTask;
    long long idleTotal = 0;
    int idleCount = 0;

    for (const auto& s : summarizeSessions(records)) {
        const std::int64_t day = localDayNumber(s.start);
        focusByDay[day] += s.focusSeconds;
        focusByHour[static_cast<std::size_t>(localHour(s.start))] += s.focusSeconds;
        if (!s.task.empty()) focusByTask[s.task] += s.focusSeconds;
        if (day >= st.firstDay && day <= today) st.dailyFocusSeconds[static_cast<std::size_t>(day - st.firstDay)] += s.focusSeconds;
        if (day == today) {
            st.todayFocusSeconds += s.focusSeconds;
            ++st.todaySessions;
        }
        ++st.totalSessions;
        st.totalFocusSeconds += s.focusSeconds;
        st.longestSessionSeconds = std::max(st.longestSessionSeconds, s.focusSeconds);
        if (s.hyperfocus) ++st.hyperfocusSessions;
        if (s.rating == Rating::Good) ++st.good;
        if (s.rating == Rating::Okay) ++st.okay;
        if (s.rating == Rating::Bad) ++st.bad;
        if (s.sawIdle) {
            idleTotal += s.idleSeconds;
            ++idleCount;
        }
    }

    if (st.totalSessions > 0) st.avgSessionSeconds = st.totalFocusSeconds / st.totalSessions;
    if (idleCount > 0) st.avgIdleSeconds = static_cast<int>(idleTotal / idleCount);

    // A streak isn't broken yet just because today hasn't started.
    std::int64_t day = focusByDay.count(today) ? today : today - 1;
    while (focusByDay.count(day)) {
        ++st.streakDays;
        --day;
    }

    int best = 0;
    for (int h = 0; h < 24; ++h) {
        if (focusByHour[static_cast<std::size_t>(h)] > best) {
            best = focusByHour[static_cast<std::size_t>(h)];
            st.bestHour = h;
        }
    }

    st.topTasks.assign(focusByTask.begin(), focusByTask.end());
    std::stable_sort(st.topTasks.begin(), st.topTasks.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
    if (st.topTasks.size() > 5) st.topTasks.resize(5);
    return st;
}

std::string sparkline(const std::vector<int>& values) {
    static const char* bars[] = {"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    const int maxValue = values.empty() ? 0 : *std::max_element(values.begin(), values.end());
    std::string out;
    for (int v : values) {
        if (v <= 0 || maxValue <= 0) {
            out += "·";
            continue;
        }
        const int idx = std::clamp(static_cast<int>((static_cast<long long>(v) * 8 - 1) / maxValue), 0, 7);
        out += bars[idx];
    }
    return out;
}

} // namespace adhd
