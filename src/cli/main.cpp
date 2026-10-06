// ADHD Pomodoro: a focus timer that bends to how ADHD attention actually works.
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "cli/App.h"
#include "cli/Render.h"
#include "cli/StatsView.h"
#include "cli/Terminal.h"
#include "core/Config.h"
#include "core/Journal.h"
#include "core/ParkingLot.h"
#include "core/Paths.h"
#include "core/Stats.h"
#include "core/TimeUtil.h"

#ifndef ADHD_VERSION
#define ADHD_VERSION "dev"
#endif

namespace {

void usage() {
    std::cout <<
        "ADHD Pomodoro " ADHD_VERSION ": a focus timer that adapts to you.\n"
        "\n"
        "Usage:\n"
        "  adhd-pomodoro [options]          run the timer\n"
        "  adhd-pomodoro stats [--days N]   your focus stats (default: last 7 days)\n"
        "  adhd-pomodoro parked             list parked thoughts\n"
        "  adhd-pomodoro where              show where settings and logs are stored\n"
        "\n"
        "Options:\n"
        "  -t, --task TEXT     name what you're working on\n"
        "  -f, --focus MIN     set the focus length (saved)\n"
        "  -r, --rest MIN      set the rest length (saved)\n"
        "      --data-dir DIR  store data in DIR (or set ADHD_POMODORO_HOME)\n"
        "  -h, --help          show this help\n"
        "  -v, --version       show the version\n"
        "\n"
        "Inside the timer press ? for help.\n";
}

int fail(const std::string& message) {
    std::cerr << "adhd-pomodoro: " << message << "\n(try --help)\n";
    return 2;
}

bool toInt(const std::string& s, int& out) {
    char* end = nullptr;
    const long v = std::strtol(s.c_str(), &end, 10);
    if (s.empty() || *end != '\0' || v < 1 || v > 1000) return false;
    out = static_cast<int>(v);
    return true;
}

} // namespace

int main(int argc, char** argv) {
    using namespace adhd;

    std::vector<std::string> args(argv + 1, argv + argc);
    std::string command, dataDir, task;
    int focus = 0, rest = 0, days = 7;

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        auto value = [&](std::string& out) {
            if (i + 1 >= args.size()) return false;
            out = args[++i];
            return true;
        };
        std::string v;
        if (a == "-h" || a == "--help") {
            usage();
            return 0;
        } else if (a == "-v" || a == "--version") {
            std::cout << "adhd-pomodoro " ADHD_VERSION "\n";
            return 0;
        } else if (a == "-t" || a == "--task") {
            if (!value(task)) return fail(a + " needs a value");
        } else if (a == "--data-dir") {
            if (!value(dataDir)) return fail(a + " needs a directory");
        } else if (a == "-f" || a == "--focus") {
            if (!value(v) || !toInt(v, focus)) return fail(a + " needs a number of minutes");
        } else if (a == "-r" || a == "--rest") {
            if (!value(v) || !toInt(v, rest)) return fail(a + " needs a number of minutes");
        } else if (a == "--days") {
            if (!value(v) || !toInt(v, days)) return fail(a + " needs a number of days");
        } else if (!a.empty() && a[0] == '-') {
            return fail("unknown option " + a);
        } else if (command.empty()) {
            command = a;
        } else {
            return fail("unexpected argument " + a);
        }
    }

    ui::Terminal::enableUtf8AndColors();
    const Paths paths = Paths::resolve(dataDir);
    if (!paths.ensureDir()) return fail("cannot create data directory " + paths.dir);

    bool existed = false;
    Config cfg = Config::load(paths.config, &existed);
    if (!existed) {
        // Carry over v1 settings if they're lying around in the current directory.
        if (std::ifstream legacy("settings.txt"); legacy) cfg = Config::parse(legacy);
        cfg.save(paths.config);
    }
    if (focus > 0) cfg.focusMinutes = focus;
    if (rest > 0) cfg.restMinutes = rest;
    if (focus > 0 || rest > 0) {
        cfg.clamp();
        cfg.save(paths.config);
    }

    if (command == "stats") {
        const bool color = ui::Terminal::stdoutIsTty();
        int skipped = 0;
        const Journal journal(paths.journal);
        const auto records = journal.readAll(&skipped);
        const Stats st = computeStats(records, localDayNumber(std::time(nullptr)), days);
        std::cout << (color ? ui::ansi::bold : "") << "ADHD Pomodoro stats" << (color ? ui::ansi::reset : "")
                  << "\n\n";
        for (const auto& line : ui::statsReport(st, color)) std::cout << line << "\n";
        std::cout << "\n" << (color ? ui::ansi::dim : "") << "journal: " << paths.journal;
        if (skipped > 0) std::cout << " (" << skipped << " unreadable lines skipped)";
        std::cout << (color ? ui::ansi::reset : "") << "\n";
        return 0;
    }
    if (command == "parked") {
        const auto items = ParkingLot(paths.parking).openItems();
        if (items.empty()) std::cout << "Nothing parked.\n";
        for (const auto& item : items) std::cout << "- [ ] " << item << "\n";
        return 0;
    }
    if (command == "where") {
        std::cout << "data dir   " << paths.dir << "\n"
                  << "settings   " << paths.config << "\n"
                  << "journal    " << paths.journal << "\n"
                  << "parked     " << paths.parking << "\n";
        return 0;
    }
    if (!command.empty() && command != "run") return fail("unknown command " + command);

    if (!ui::Terminal::stdinIsTty() || !ui::Terminal::stdoutIsTty())
        return fail("the timer needs an interactive terminal (stats/parked/where work anywhere)");

    ui::App app(paths, cfg);
    app.setTask(task);
    return app.run();
}
