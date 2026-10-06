// Dependency-free unit tests. Run with `ctest` or ./adhd_tests.
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "cli/Render.h"
#include "core/Clock.h"
#include "core/Config.h"
#include "core/Engine.h"
#include "core/Journal.h"
#include "core/ParkingLot.h"
#include "core/Stats.h"
#include "core/TimeUtil.h"

using namespace adhd;

// ---------------------------------------------------------------- tiny framework

namespace {
struct TestCase {
    const char* name;
    std::function<void()> fn;
};
std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}
int g_failures = 0;
struct Registrar {
    Registrar(const char* name, std::function<void()> fn) { registry().push_back({name, std::move(fn)}); }
};
} // namespace

#define TEST(name)                                         \
    static void name();                                    \
    static const Registrar registrar_##name(#name, name);  \
    static void name()

#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (!(cond)) {                                                                    \
            ++g_failures;                                                                 \
            std::cerr << "  " << __FILE__ << ":" << __LINE__ << ": CHECK(" #cond ") failed\n"; \
        }                                                                                 \
    } while (0)

#define CHECK_EQ(a, b)                                                                     \
    do {                                                                                   \
        const auto va = (a);                                                               \
        const auto vb = (b);                                                               \
        if (!(va == vb)) {                                                                 \
            ++g_failures;                                                                  \
            std::cerr << "  " << __FILE__ << ":" << __LINE__ << ": CHECK_EQ(" #a ", " #b ") failed: " \
                      << va << " != " << vb << "\n";                                       \
        }                                                                                  \
    } while (0)

// ---------------------------------------------------------------- helpers

namespace {

struct Recorder {
    std::vector<Engine::Event> events;
    std::vector<PhaseRecord> records() const {
        std::vector<PhaseRecord> out;
        for (const auto& e : events)
            if (e.type == Engine::Event::Type::PhaseCompleted) out.push_back(e.record);
        return out;
    }
    int count(Engine::Event::Type t) const {
        int n = 0;
        for (const auto& e : events) n += e.type == t;
        return n;
    }
};

struct Fixture {
    Config cfg;
    ManualClock clock;
    Engine engine{cfg, clock};
    Recorder rec;
    Fixture() {
        cfg.focusMinutes = 15;
        cfg.restMinutes = 5;
        engine.setListener([this](const Engine::Event& e) { rec.events.push_back(e); });
    }
    void advance(int seconds) {
        clock.advanceSeconds(seconds);
        engine.tick();
    }
};

std::time_t localTime(int y, int mo, int d, int h, int mi) {
    std::tm tm{};
    tm.tm_year = y - 1900;
    tm.tm_mon = mo - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = mi;
    tm.tm_isdst = -1;
    return std::mktime(&tm);
}

std::filesystem::path tempDir() {
    auto dir = std::filesystem::temp_directory_path() /
               ("adhd_tests_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(std::rand()));
    std::filesystem::create_directories(dir);
    return dir;
}

} // namespace

// ---------------------------------------------------------------- time formatting

TEST(formats_clock_and_durations) {
    CHECK_EQ(formatClock(0), std::string("00:00"));
    CHECK_EQ(formatClock(907), std::string("15:07"));
    CHECK_EQ(formatClock(3723), std::string("1:02:03"));
    CHECK_EQ(formatClock(-5), std::string("00:00"));
    CHECK_EQ(formatDuration(42), std::string("42s"));
    CHECK_EQ(formatDuration(25 * 60), std::string("25m"));
    CHECK_EQ(formatDuration(2 * 3600), std::string("2h"));
    CHECK_EQ(formatDuration(85 * 60 + 10), std::string("1h 25m"));
}

TEST(civil_days_and_weekdays) {
    CHECK_EQ(daysFromCivil(1970, 1, 1), 0);
    CHECK_EQ(daysFromCivil(2000, 3, 1), 11017);
    CHECK_EQ(std::string(weekdayShort(daysFromCivil(2026, 10, 6))), std::string("Tue"));
    std::time_t t = 0;
    CHECK(parseLocalTime("2026-10-06 14:30:00", t));
    CHECK_EQ(formatLocalTime(t), std::string("2026-10-06 14:30:00"));
    CHECK(!parseLocalTime("not a date", t));
}

// ---------------------------------------------------------------- config

TEST(config_reads_legacy_v1_format) {
    std::istringstream in("25 7");
    const Config c = Config::parse(in);
    CHECK_EQ(c.focusMinutes, 25);
    CHECK_EQ(c.restMinutes, 7);
    CHECK(c.adaptive); // everything else keeps defaults
}

TEST(config_parses_keys_hooks_and_clamps) {
    std::istringstream in(
        "# comment\n"
        "focus_minutes = 30\n"
        "rest_minutes=9999\n"
        "adaptive = off\n"
        "bell = yes\n"
        "nudge_minutes = banana\n"
        "hook.focus = playerctl play\n"
        "hook.rest = echo {phase}\n"
        "unknown_key = 1\n");
    const Config c = Config::parse(in);
    CHECK_EQ(c.focusMinutes, 30);
    CHECK_EQ(c.restMinutes, 120); // clamped
    CHECK(!c.adaptive);
    CHECK(c.bell);
    CHECK_EQ(c.nudgeMinutes, Config{}.nudgeMinutes); // bad value ignored
    CHECK_EQ(c.hooks.at("focus"), std::string("playerctl play"));
    CHECK_EQ(c.hooks.at("rest"), std::string("echo {phase}"));
}

TEST(config_round_trips) {
    Config c;
    c.focusMinutes = 22;
    c.notify = true;
    c.restPercent = 33;
    c.hooks["hyperfocus"] = "say go";
    std::stringstream buf;
    c.write(buf);
    const Config back = Config::parse(buf);
    CHECK_EQ(back.focusMinutes, 22);
    CHECK(back.notify);
    CHECK_EQ(back.restPercent, 33);
    CHECK_EQ(back.hooks.at("hyperfocus"), std::string("say go"));
}

// ---------------------------------------------------------------- engine

TEST(engine_full_cycle) {
    Fixture f;
    CHECK(f.engine.phase() == Phase::Ready);
    CHECK(!f.engine.rate(Rating::Good)); // nothing to rate yet
    CHECK(f.engine.startFocus());
    CHECK(f.engine.phase() == Phase::Focus);
    CHECK_EQ(f.engine.remainingSeconds(), 15 * 60);

    f.advance(15 * 60 - 1);
    CHECK(f.engine.phase() == Phase::Focus);
    CHECK_EQ(f.engine.remainingSeconds(), 1);

    f.advance(1);
    CHECK(f.engine.phase() == Phase::ExtendedFocus); // not interrupted, just extended
    CHECK(f.rec.events.back().automatic);

    f.advance(5 * 60);
    CHECK_EQ(f.engine.sessionFocusSeconds(), 20 * 60);
    CHECK_EQ(f.rec.count(Engine::Event::Type::Nudge), 1); // nudge every 3 minutes

    CHECK(f.engine.rate(Rating::Good));
    CHECK(f.engine.phase() == Phase::Rest);
    CHECK_EQ(f.cfg.focusMinutes, 17); // adapted up after a full, good session
    CHECK_EQ(f.engine.targetSeconds(), 5 * 60);

    f.advance(5 * 60);
    CHECK(f.engine.phase() == Phase::Idle);
    f.advance(2 * 60);
    CHECK(f.engine.startFocus());
    CHECK_EQ(f.engine.remainingSeconds(), 17 * 60);

    const auto r = f.rec.records();
    CHECK_EQ(r.size(), std::size_t{4});
    CHECK(r[0].phase == Phase::Focus && r[0].seconds == 900);
    CHECK(r[1].phase == Phase::ExtendedFocus && r[1].seconds == 300 && r[1].rating == Rating::Good);
    CHECK(r[2].phase == Phase::Rest && r[2].seconds == 300);
    CHECK(r[3].phase == Phase::Idle && r[3].seconds == 120);
    CHECK_EQ(r[0].sessionId, r[3].sessionId); // rest and idle belong to the session before them
}

TEST(engine_pause_freezes_time) {
    Fixture f;
    f.engine.startFocus();
    f.advance(5 * 60);
    CHECK(f.engine.togglePause());
    f.advance(60 * 60);
    CHECK(f.engine.phase() == Phase::Focus);
    CHECK_EQ(f.engine.remainingSeconds(), 10 * 60);
    CHECK(f.engine.togglePause());
    f.advance(10 * 60);
    CHECK(f.engine.phase() == Phase::ExtendedFocus);
}

TEST(engine_overshoot_counts_as_extended_focus) {
    Fixture f;
    f.engine.startFocus();
    f.advance(40 * 60); // e.g. laptop asleep
    CHECK(f.engine.phase() == Phase::ExtendedFocus);
    CHECK_EQ(f.engine.elapsedSeconds(), 25 * 60);
    CHECK_EQ(f.engine.sessionFocusSeconds(), 40 * 60);
}

TEST(engine_adaptation_rules) {
    Fixture f;
    f.cfg.minFocusMinutes = 10;
    f.cfg.maxFocusMinutes = 16;

    // Good but ended early: no growth.
    f.engine.startFocus();
    f.advance(60);
    f.engine.rate(Rating::Good);
    CHECK_EQ(f.cfg.focusMinutes, 15);

    // Good after jumping into hyperfocus early still needs the full focus time.
    f.engine.startFocus();
    f.advance(60);
    f.engine.enterHyperfocus();
    f.advance(60);
    f.engine.rate(Rating::Good);
    CHECK_EQ(f.cfg.focusMinutes, 15);

    // Bad: shrink, also when ended early.
    f.engine.startFocus();
    f.advance(60);
    f.engine.rate(Rating::Bad);
    CHECK_EQ(f.cfg.focusMinutes, 13);

    // Okay: unchanged.
    f.engine.startFocus();
    f.advance(13 * 60);
    f.engine.rate(Rating::Okay);
    CHECK_EQ(f.cfg.focusMinutes, 13);

    // Good after the full timer, capped at the upper bound.
    for (int i = 0; i < 3; ++i) {
        f.engine.startFocus();
        f.advance(f.cfg.focusMinutes * 60);
        f.engine.rate(Rating::Good);
    }
    CHECK_EQ(f.cfg.focusMinutes, 16);

    // Bad sessions bottom out at the lower bound.
    for (int i = 0; i < 5; ++i) {
        f.engine.startFocus();
        f.engine.rate(Rating::Bad);
    }
    CHECK_EQ(f.cfg.focusMinutes, 10);

    // Adaptation off: nothing moves.
    f.cfg.adaptive = false;
    f.engine.startFocus();
    f.engine.rate(Rating::Bad);
    CHECK_EQ(f.cfg.focusMinutes, 10);
}

TEST(engine_tiny_start_never_adapts) {
    Fixture f;
    f.cfg.tinyStartMinutes = 5;
    CHECK(f.engine.startTinyFocus());
    CHECK(f.engine.isTinyStart());
    CHECK_EQ(f.engine.remainingSeconds(), 5 * 60);
    f.advance(5 * 60);
    CHECK(f.engine.phase() == Phase::ExtendedFocus); // tiny start that turned into real work
    f.engine.rate(Rating::Good);
    CHECK_EQ(f.cfg.focusMinutes, 15);
}

TEST(engine_hyperfocus_earns_proportional_rest) {
    Fixture f;
    f.engine.startFocus();
    f.advance(60);
    CHECK(f.engine.enterHyperfocus());
    CHECK(f.engine.phase() == Phase::Hyperfocus);
    CHECK(!f.engine.enterHyperfocus());
    for (int minute = 0; minute < 90; ++minute) f.advance(60);
    CHECK_EQ(f.rec.count(Engine::Event::Type::Nudge), 3); // body check every 30 min
    CHECK(f.engine.rate(Rating::None));
    // 91 min * 20% = 18.2 -> 18 min, above the 5 min minimum, under the 30 min cap
    CHECK_EQ(f.engine.targetSeconds(), 18 * 60);
    CHECK_EQ(f.cfg.focusMinutes, 15); // no rating, no adaptation
}

TEST(engine_missed_nudges_collapse_into_one) {
    Fixture f;
    f.engine.startFocus();
    f.engine.enterHyperfocus();
    f.advance(90 * 60); // one tick after a long gap (e.g. suspend): one nudge, not three
    CHECK_EQ(f.rec.count(Engine::Event::Type::Nudge), 1);
    f.advance(29 * 60);
    CHECK_EQ(f.rec.count(Engine::Event::Type::Nudge), 1);
    f.advance(60);
    CHECK_EQ(f.rec.count(Engine::Event::Type::Nudge), 2);
}

TEST(engine_rest_math) {
    Fixture f;
    CHECK_EQ(f.engine.restSecondsFor(10 * 60), 5 * 60);   // floor at rest_minutes
    CHECK_EQ(f.engine.restSecondsFor(60 * 60), 12 * 60);  // 20%
    CHECK_EQ(f.engine.restSecondsFor(600 * 60), 30 * 60); // capped
    f.cfg.restPercent = 0;
    CHECK_EQ(f.engine.restSecondsFor(600 * 60), 5 * 60);
}

TEST(engine_stop_logs_partial_and_resets) {
    Fixture f;
    CHECK(!f.engine.stop());
    f.engine.setTask("taxes");
    f.engine.startFocus();
    f.advance(7 * 60);
    CHECK(f.engine.stop());
    CHECK(f.engine.phase() == Phase::Ready);
    const auto r = f.rec.records();
    CHECK_EQ(r.size(), std::size_t{1});
    CHECK_EQ(r[0].seconds, 7 * 60);
    CHECK_EQ(r[0].task, std::string("taxes"));
    CHECK(!f.engine.togglePause());
}

TEST(engine_sessions_get_unique_ids) {
    Fixture f;
    f.engine.startFocus();
    f.engine.rate(Rating::Okay);
    f.engine.startFocus(); // same wall-clock second
    f.engine.stop();
    const auto r = f.rec.records();
    CHECK(r.front().sessionId != r.back().sessionId);
}

// ---------------------------------------------------------------- journal

TEST(csv_round_trip_with_awkward_task_names) {
    PhaseRecord r;
    r.sessionId = 42;
    r.startedAt = localTime(2026, 10, 6, 9, 15);
    r.phase = Phase::Hyperfocus;
    r.seconds = 3600;
    r.rating = Rating::Good;
    r.task = "write \"chapter 2\", then edit";
    PhaseRecord back;
    CHECK(Journal::fromCsvLine(Journal::toCsvLine(r), back));
    CHECK_EQ(back.sessionId, 42);
    CHECK_EQ(back.startedAt, r.startedAt);
    CHECK(back.phase == Phase::Hyperfocus);
    CHECK_EQ(back.seconds, 3600);
    CHECK(back.rating == Rating::Good);
    CHECK_EQ(back.task, r.task);
    CHECK(!Journal::fromCsvLine("garbage", back));
    CHECK(!Journal::fromCsvLine("1,2026-10-06 09:15:00,napping,10,,", back));
}

TEST(journal_appends_and_reads_back) {
    const auto dir = tempDir();
    const Journal j((dir / "sessions.csv").string());
    PhaseRecord r;
    r.sessionId = 1;
    r.startedAt = localTime(2026, 10, 6, 9, 0);
    r.phase = Phase::Focus;
    r.seconds = 900;
    CHECK(j.append(r));
    r.phase = Phase::Rest;
    r.seconds = 300;
    CHECK(j.append(r));
    int skipped = -1;
    const auto all = j.readAll(&skipped);
    CHECK_EQ(all.size(), std::size_t{2});
    CHECK_EQ(skipped, 0);
    CHECK(all[1].phase == Phase::Rest);
    std::filesystem::remove_all(dir);
}

// ---------------------------------------------------------------- stats

TEST(stats_summarize_days_streaks_and_ratings) {
    auto rec = [](std::int64_t id, std::time_t at, Phase p, int secs, Rating r = Rating::None,
                  std::string task = "") { return PhaseRecord{id, at, p, secs, r, std::move(task)}; };
    const std::time_t mon = localTime(2026, 10, 5, 10, 0);
    const std::time_t tue = localTime(2026, 10, 6, 10, 0);
    const std::time_t tueAfternoon = localTime(2026, 10, 6, 15, 0);
    const std::time_t lastWeek = localTime(2026, 9, 28, 9, 0);

    const std::vector<PhaseRecord> records = {
        rec(1, lastWeek, Phase::Focus, 600, Rating::Bad),
        rec(2, mon, Phase::Focus, 900),
        rec(2, mon, Phase::ExtendedFocus, 300, Rating::Good, "thesis"),
        rec(2, mon, Phase::Rest, 300),
        rec(2, mon, Phase::Idle, 240),
        rec(3, tue, Phase::Focus, 900),
        rec(3, tue, Phase::Hyperfocus, 3600, Rating::Good, "thesis"),
        rec(3, tue, Phase::Rest, 900),
        rec(3, tue, Phase::Idle, 60),
        rec(4, tueAfternoon, Phase::Focus, 300, Rating::Okay, "email"),
        rec(5, tueAfternoon, Phase::Rest, 300), // rest without focus: not a session
        rec(6, tueAfternoon, Phase::Focus, 4),  // accidental start: not a session
    };
    const Stats st = computeStats(records, daysFromCivil(2026, 10, 6), 7);

    CHECK_EQ(st.totalSessions, 4);
    CHECK_EQ(st.todaySessions, 2);
    CHECK_EQ(st.todayFocusSeconds, 900 + 3600 + 300);
    CHECK_EQ(st.streakDays, 2);
    CHECK_EQ(st.good, 2);
    CHECK_EQ(st.okay, 1);
    CHECK_EQ(st.bad, 1);
    CHECK_EQ(st.hyperfocusSessions, 1);
    CHECK_EQ(st.longestSessionSeconds, 4500);
    CHECK_EQ(st.avgIdleSeconds, 150);
    CHECK_EQ(st.bestHour, 10);
    CHECK_EQ(st.dailyFocusSeconds.size(), std::size_t{7});
    CHECK_EQ(st.dailyFocusSeconds[6], 4800); // today
    CHECK_EQ(st.dailyFocusSeconds[5], 1200); // yesterday
    CHECK_EQ(st.dailyFocusSeconds[0], 0);    // last week's session is 8 days back
    CHECK(!st.topTasks.empty());
    CHECK_EQ(st.topTasks[0].first, std::string("thesis"));
    CHECK_EQ(st.topTasks[0].second, 1200 + 4500);

    // Streak survives a day that hasn't had a session yet.
    const Stats tomorrow = computeStats(records, daysFromCivil(2026, 10, 7), 7);
    CHECK_EQ(tomorrow.streakDays, 2);
    const Stats later = computeStats(records, daysFromCivil(2026, 10, 9), 7);
    CHECK_EQ(later.streakDays, 0);
}

TEST(stats_empty_and_sparkline) {
    const Stats st = computeStats({}, 20000, 7);
    CHECK_EQ(st.totalSessions, 0);
    CHECK_EQ(st.streakDays, 0);
    CHECK_EQ(st.bestHour, -1);
    CHECK_EQ(sparkline({0, 1, 8}), std::string("·▁█"));
    CHECK_EQ(sparkline({}), std::string(""));
}

// ---------------------------------------------------------------- parking lot

TEST(parking_lot_collects_open_items) {
    const auto dir = tempDir();
    const ParkingLot lot((dir / "parked.md").string());
    CHECK(lot.openItems().empty());
    CHECK(lot.add("reply to Sam", std::time(nullptr)));
    CHECK(lot.add("look up\nthat movie", std::time(nullptr)));
    CHECK(!lot.add("   ", std::time(nullptr)));
    const auto items = lot.openItems();
    CHECK_EQ(items.size(), std::size_t{2});
    CHECK_EQ(items[0], std::string("reply to Sam"));
    CHECK_EQ(items[1], std::string("look up that movie"));
    std::filesystem::remove_all(dir);
}

// ---------------------------------------------------------------- rendering helpers

TEST(render_helpers) {
    using namespace adhd::ui;
    CHECK_EQ(visibleWidth(std::string(ansi::bold) + "héllo" + ansi::reset), 5);
    CHECK_EQ(visibleWidth("\x1b]0;title\a12"), 2);
    const auto rows = bigText("12:34");
    CHECK_EQ(rows.size(), std::size_t{5});
    for (const auto& row : rows) CHECK_EQ(visibleWidth(row), visibleWidth(rows[0]));
    CHECK_EQ(visibleWidth(progressBar(0.37, 20, ansi::cyan)), 20);
    CHECK_EQ(visibleWidth(progressBar(1.0, 20, ansi::cyan)), 20);
    CHECK_EQ(visibleWidth(truncate("abcdefghij", 5)), 5);
    const auto lines = wrap("one two three four five", 9);
    CHECK_EQ(lines.size(), std::size_t{3});
    for (const auto& l : lines) CHECK(visibleWidth(l) <= 9);
    const auto items = wrapItems("s start · m tiny · q quit", 17);
    CHECK_EQ(items.size(), std::size_t{2});
    CHECK_EQ(items[0], std::string("s start · m tiny"));
    CHECK_EQ(items[1], std::string("q quit"));
}

// ---------------------------------------------------------------- runner

int main() {
    int failedTests = 0;
    for (const auto& t : registry()) {
        const int before = g_failures;
        t.fn();
        const bool ok = g_failures == before;
        failedTests += !ok;
        std::cout << (ok ? "  ok    " : "  FAIL  ") << t.name << "\n";
    }
    std::cout << "\n" << registry().size() - static_cast<std::size_t>(failedTests) << "/" << registry().size()
              << " tests passed\n";
    return failedTests == 0 ? 0 : 1;
}
