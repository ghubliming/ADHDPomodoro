#include "cli/App.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>

#include "cli/Render.h"
#include "cli/StatsView.h"
#include "core/TimeUtil.h"

namespace adhd::ui {

namespace {

constexpr int kMaxWidth = 64;
constexpr int kBodyHeight = 24; // nominal frame height, used for vertical centering
constexpr std::int64_t kToastMs = 6000;

std::string m(int minutes) { return std::to_string(minutes) + "m"; }

std::string pick(const std::vector<std::string>& options, int seed) {
    return options[static_cast<std::size_t>(seed) % options.size()];
}

bool isStartable(Phase p) { return p == Phase::Ready || p == Phase::Rest || p == Phase::Idle; }

void shell(const std::string& command) {
    const int rc = std::system(command.c_str());
    (void)rc;
}

} // namespace

App::App(Paths paths, Config config)
    : paths_(std::move(paths)),
      cfg_(std::move(config)),
      engine_(cfg_, clock_),
      journal_(paths_.journal),
      parking_(paths_.parking) {
    engine_.setListener([this](const Engine::Event& e) { onEvent(e); });
}

// ---------------------------------------------------------------- events

void App::onEvent(const Engine::Event& e) {
    using T = Engine::Event::Type;
    switch (e.type) {
        case T::PhaseCompleted:
            if (e.record.seconds > 0 || e.record.rating != Rating::None) {
                journal_.append(e.record);
                refreshStats();
            }
            break;

        case T::PhaseChanged:
            runHook(e.to);
            if (e.automatic && e.to == Phase::ExtendedFocus) {
                bell();
                notify("Focus timer done. Keep going, or rate the session when you surface.");
                toast("Target reached! Still in the zone? Keep going. Surfacing? Rate it: g / o / b");
            } else if (e.automatic && e.to == Phase::Idle) {
                bell();
                notify("Break is over. Ready for the next focus?");
                toast("Break's over. s to start, or m for a tiny " + m(cfg_.tinyStartMinutes) + " start.");
            }
            break;

        case T::Nudge:
            bell();
            if (e.from == Phase::ExtendedFocus) {
                toast("+" + formatDuration(engine_.elapsedSeconds()) +
                      " past your target. Rate it whenever you surface.");
            } else if (e.from == Phase::Idle) {
                const std::vector<std::string> idle = {
                    "Idle a while. What's the smallest next step? Do just that.",
                    "Starting is the hard part. m = just " + m(cfg_.tinyStartMinutes) + ".",
                    "Still here? No guilt. Press s when you're ready.",
                };
                toast(pick(idle, e.nudgeCount - 1));
            } else if (e.from == Phase::Hyperfocus) {
                notify("Hyperfocus check-in: water, stretch, eyes.");
                toast("Check-in: " + formatDuration(engine_.sessionFocusSeconds()) +
                      " deep. Water? Stretch? Still the right task?");
            }
            break;

        case T::FocusAdjusted:
            cfg_.save(paths_.config);
            adjustNote_ = "Next focus " + m(e.newFocusMinutes) + " (was " + m(e.oldFocusMinutes) + ").";
            break;
    }
}

void App::refreshStats() {
    stats_ = computeStats(journal_.readAll(), localDayNumber(clock_.wallTime()));
    parkedCount_ = static_cast<int>(parking_.openItems().size());
}

int App::todayFocusSeconds() const {
    return stats_.todayFocusSeconds + (isFocusPhase(engine_.phase()) ? engine_.elapsedSeconds() : 0);
}

void App::toast(std::string message) {
    toast_ = std::move(message);
    toastUntilMs_ = nowMs() + kToastMs;
}

void App::bell() {
    if (cfg_.bell && term_) term_->write("\a");
}

void App::notify(const char* message) const {
    // `message` is always one of our own literals (no quotes), never user text.
    if (!cfg_.notify) return;
#if defined(__APPLE__)
    shell(std::string("osascript -e 'display notification \"") + message +
          "\" with title \"ADHD Pomodoro\"' >/dev/null 2>&1 &");
#elif !defined(_WIN32)
    shell(std::string("command -v notify-send >/dev/null 2>&1 && notify-send 'ADHD Pomodoro' '") + message +
          "' >/dev/null 2>&1 &");
#else
    (void)message;
#endif
}

void App::runHook(Phase phase) const {
    const auto it = cfg_.hooks.find(phaseName(phase));
    if (it == cfg_.hooks.end()) return;
    std::string cmd = it->second;
    const std::string placeholder = "{phase}";
    for (auto pos = cmd.find(placeholder); pos != std::string::npos; pos = cmd.find(placeholder, pos))
        cmd.replace(pos, placeholder.size(), phaseName(phase));
#ifdef _WIN32
    shell("start \"\" /b " + cmd + " >NUL 2>&1");
#else
    shell("(" + cmd + ") </dev/null >/dev/null 2>&1 &");
#endif
}

// ---------------------------------------------------------------- input

void App::handleKey(const Key& k) {
    switch (mode_) {
        case Mode::Normal: handleNormalKey(k); break;
        case Mode::Input: handleInputKey(k); break;
        case Mode::Settings: handleSettingsKey(k); break;
        case Mode::Overlay: mode_ = Mode::Normal; break;
        case Mode::ConfirmQuit:
            if (k.ch() == 'y' || k.ch() == 'Y' || k.ch() == 'q') quit_ = true;
            mode_ = Mode::Normal;
            break;
    }
}

void App::start(bool tiny) {
    if (!(tiny ? engine_.startTinyFocus() : engine_.startFocus())) return;
    if (tiny) toast("Tiny start: just " + m(cfg_.tinyStartMinutes) + ". If it flows, keep going.");
    else toast(engine_.task().empty() ? "Go. One thing at a time. (t names the task)" : "Go: " + engine_.task());
}

void App::rate(Rating r) {
    adjustNote_.clear();
    if (!engine_.rate(r)) return;
    const std::string rest = "Rest " + formatDuration(engine_.targetSeconds()) + ".";
    std::string msg;
    switch (r) {
        case Rating::Good: msg = "Nice work! " + rest; break;
        case Rating::Okay: msg = "Solid. " + rest; break;
        case Rating::Bad: msg = "Rough one. That's data, not failure. " + rest; break;
        case Rating::None: msg = rest; break;
    }
    if (!adjustNote_.empty()) msg += " " + adjustNote_;
    toast(msg);
}

void App::handleNormalKey(const Key& k) {
    const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(k.ch())));
    const Phase p = engine_.phase();
    switch (c) {
        case ' ':
            if (isStartable(p) && p != Phase::Rest) start(false);
            else engine_.togglePause();
            break;
        case 's': start(false); break;
        case 'm': start(true); break;
        case 'p': engine_.togglePause(); break;
        case 'g': rate(Rating::Good); break;
        case 'o': rate(Rating::Okay); break;
        case 'b': rate(Rating::Bad); break;
        case 'r': rate(Rating::None); break;
        case 'h':
            if (engine_.enterHyperfocus()) toast("Hyperfocus. Go deep; check-ins every " + m(cfg_.hyperfocusCheckMinutes) + ".");
            else showHelp();
            break;
        case 'd': beginInput(InputPurpose::Park); break;
        case 't': beginInput(InputPurpose::Task); break;
        case 'v': showParked(); break;
        case 'i': showStats(); break;
        case 'c':
            mode_ = Mode::Settings;
            break;
        case 'x':
            if (engine_.stop()) toast("Stopped. What you did is logged, and it counts.");
            break;
        case '?': showHelp(); break;
        case 'q':
            if (p == Phase::Ready) quit_ = true;
            else mode_ = Mode::ConfirmQuit;
            break;
        default: break;
    }
}

void App::beginInput(InputPurpose purpose) {
    inputPurpose_ = purpose;
    input_ = purpose == InputPurpose::Task ? engine_.task() : "";
    mode_ = Mode::Input;
}

void App::handleInputKey(const Key& k) {
    switch (k.type) {
        case KeyType::Escape: mode_ = Mode::Normal; break;
        case KeyType::Backspace:
            while (!input_.empty() && (static_cast<unsigned char>(input_.back()) & 0xC0) == 0x80) input_.pop_back();
            if (!input_.empty()) input_.pop_back();
            break;
        case KeyType::Char:
            if (input_.size() < 200) input_ += k.text;
            break;
        case KeyType::Enter:
            mode_ = Mode::Normal;
            if (inputPurpose_ == InputPurpose::Park) {
                if (parking_.add(input_, clock_.wallTime())) {
                    ++parkedCount_;
                    toast("Parked. Back to it! (" + std::to_string(parkedCount_) + " waiting for your break)");
                }
            } else {
                engine_.setTask(input_);
                toast(input_.empty() ? "Task cleared." : "Task: " + input_);
            }
            break;
        default: break;
    }
}

std::vector<App::SettingRow> App::settingRows() {
    return {
        {"Focus length", &cfg_.focusMinutes, nullptr, 1, 240, "min"},
        {"Rest length", &cfg_.restMinutes, nullptr, 1, 120, "min"},
        {"Tiny start", &cfg_.tinyStartMinutes, nullptr, 1, 60, "min"},
        {"Adaptive focus", nullptr, &cfg_.adaptive, 0, 0, ""},
        {"  step", &cfg_.adaptStepMinutes, nullptr, 0, 30, "min"},
        {"  lower bound", &cfg_.minFocusMinutes, nullptr, 1, 240, "min"},
        {"  upper bound", &cfg_.maxFocusMinutes, nullptr, 1, 240, "min"},
        {"Proportional rest", &cfg_.restPercent, nullptr, 0, 100, "% of focus time"},
        {"Max rest", &cfg_.maxRestMinutes, nullptr, 1, 240, "min"},
        {"Nudge every", &cfg_.nudgeMinutes, nullptr, 0, 120, "min (0 = off)"},
        {"Hyperfocus check-in", &cfg_.hyperfocusCheckMinutes, nullptr, 0, 240, "min (0 = off)"},
        {"Bell", nullptr, &cfg_.bell, 0, 0, ""},
        {"Desktop notifications", nullptr, &cfg_.notify, 0, 0, ""},
    };
}

void App::handleSettingsKey(const Key& k) {
    auto rows = settingRows();
    const int n = static_cast<int>(rows.size());
    SettingRow& row = rows[static_cast<std::size_t>(settingsIndex_)];
    const char c = k.ch();
    int delta = 0;

    if (k.type == KeyType::Up || c == 'k') settingsIndex_ = (settingsIndex_ + n - 1) % n;
    else if (k.type == KeyType::Down || c == 'j') settingsIndex_ = (settingsIndex_ + 1) % n;
    else if (k.type == KeyType::Left || c == '-' || c == 'h') delta = -1;
    else if (k.type == KeyType::Right || c == '+' || c == '=' || c == 'l') delta = 1;
    else if (k.type == KeyType::Enter || c == ' ') delta = 1;
    else if (k.type == KeyType::Escape || c == 'q' || c == 'c') {
        cfg_.clamp();
        toast(cfg_.save(paths_.config) ? "Settings saved." : "Couldn't save settings to " + paths_.config);
        mode_ = Mode::Normal;
        return;
    }

    if (delta != 0) {
        if (row.flag) *row.flag = !*row.flag;
        else if (row.value) *row.value = std::clamp(*row.value + delta, row.lo, row.hi);
    }
}

void App::showOverlay(std::string title, std::vector<std::string> lines) {
    overlayTitle_ = std::move(title);
    overlayLines_ = std::move(lines);
    mode_ = Mode::Overlay;
}

void App::showHelp() {
    showOverlay("How it works", {
        "Focus counts down. When it ends you are NOT interrupted:",
        "Extended Focus counts up until you surface and rate it.",
        "Rating starts a rest. Long sessions earn longer rests.",
        "When rest ends, Idle counts how long restarting takes.",
        "",
        "Ratings tune your next focus: good after a full session",
        "+" + m(cfg_.adaptStepMinutes) + ", bad -" + m(cfg_.adaptStepMinutes) + ", okay keeps it.",
        "",
        "s / space  start focus      m  tiny start (" + m(cfg_.tinyStartMinutes) + ")",
        "space / p  pause / resume   x  stop (still logged)",
        "g  o  b    end + rate       r  end, no rating",
        "h          hyperfocus       d  park a distracting thought",
        "t          name the task    v  review parked thoughts",
        "i          stats            c  settings",
        "?          this help        q  quit",
    });
}

void App::showStats() {
    refreshStats();
    showOverlay("Stats", statsReport(stats_, true));
}

void App::showParked() {
    auto items = parking_.openItems();
    parkedCount_ = static_cast<int>(items.size());
    std::vector<std::string> lines;
    if (items.empty()) {
        lines.push_back("Nothing parked. Press d during focus when a thought pops up.");
    } else {
        const std::size_t first = items.size() > 12 ? items.size() - 12 : 0;
        if (first > 0) lines.push_back(std::string(ansi::dim) + "… " + std::to_string(first) + " older" + ansi::reset);
        for (std::size_t i = first; i < items.size(); ++i) lines.push_back("☐ " + truncate(items[i], kMaxWidth - 4));
        lines.push_back("");
        lines.push_back(std::string(ansi::dim) + "Tick them off in" + ansi::reset);
        lines.push_back(std::string(ansi::dim) + truncate(paths_.parking, kMaxWidth) + ansi::reset);
    }
    showOverlay("Parked thoughts", lines);
}

// ---------------------------------------------------------------- rendering

std::string App::phaseMessage() const {
    const int seed = engine_.elapsedSeconds() / 20; // rotate tips so they don't fade into the background
    switch (engine_.phase()) {
        case Phase::Ready:
            return pick({"Pick one small thing. Press s for a " + m(cfg_.focusMinutes) + " focus.",
                         "Can't get going? m = tiny " + m(cfg_.tinyStartMinutes) +
                             " start. Momentum comes after starting, not before."},
                        seed);
        case Phase::Focus:
            return pick({"One thing. Just this one thing.",
                         "A thought pops up? Press d to park it, then carry on.",
                         "Drifted off? No drama. Just come back.",
                         "Close the tabs you don't need right now."},
                        seed);
        case Phase::ExtendedFocus:
            return pick({"Timer's done, but you're still going. Ride the wave.",
                         "Surface whenever you like and rate it: g good · o okay · b bad."},
                        seed);
        case Phase::Hyperfocus:
            return pick({"Deep mode. Time still passes out here.",
                         "Body check: water, posture, bathroom, eyes."},
                        seed);
        case Phase::Rest:
            if (parkedCount_ > 0 && seed % 2 == 1)
                return std::to_string(parkedCount_) + " parked thought" + (parkedCount_ == 1 ? "" : "s") +
                       " waiting. v to review.";
            return pick({"Stand up. Stretch. Water. Look at something far away.",
                         "Step away from the screen if you can. Let your brain wander."},
                        seed);
        case Phase::Idle:
            return pick({"Break's over. Starting is the hardest part.",
                         "Tiny step: press m and do just " + m(cfg_.tinyStartMinutes) + ".",
                         "Not ready? Fine. s whenever you are."},
                        seed);
    }
    return "";
}

std::string App::keyHints() const {
    switch (engine_.phase()) {
        case Phase::Ready: return "s start · m tiny start · t task · i stats · c settings · ? help · q quit";
        case Phase::Focus: return "space pause · g/o/b end+rate · h hyperfocus · d park · t task · x stop";
        case Phase::ExtendedFocus: return "g good · o okay · b bad · r rest · h hyperfocus · d park · space pause";
        case Phase::Hyperfocus: return "g/o/b end+rate · r rest · d park · space pause · x stop";
        case Phase::Rest: return "s focus now · m tiny start · v parked · space pause · x stop";
        case Phase::Idle: return "s start · m tiny start · v parked · i stats · x stop";
    }
    return "";
}

void App::timerBody(std::vector<std::string>& out, int width) {
    const Phase p = engine_.phase();
    const bool paused = engine_.paused();
    const char* col = phaseColor(p);
    const std::string R = ansi::reset;

    int shown = 0;
    switch (p) {
        case Phase::Ready: shown = cfg_.focusMinutes * 60; break;
        case Phase::Focus:
        case Phase::Rest: shown = engine_.remainingSeconds(); break;
        case Phase::ExtendedFocus:
        case Phase::Hyperfocus: shown = engine_.sessionFocusSeconds(); break;
        case Phase::Idle: shown = engine_.elapsedSeconds(); break;
    }
    const std::string digitStyle = std::string(paused || p == Phase::Ready ? ansi::dim : "") + col;
    for (const auto& row : bigText(formatClock(shown))) out.push_back(center(digitStyle + row + R, width));
    out.push_back("");

    std::string label = std::string(col) + ansi::bold + "● " + phaseLabel(p) + R;
    if (p == Phase::Focus && engine_.isTinyStart()) label += std::string(ansi::dim) + " (tiny start)" + R;
    if (paused) label += std::string("  ") + ansi::bold + "PAUSED" + R;
    if (!engine_.task().empty()) label += std::string(ansi::dim) + "  · " + R + truncate(engine_.task(), 30);
    out.push_back(center(label, width));

    const int barWidth = width - 12;
    std::string bar, tail;
    switch (p) {
        case Phase::Ready:
            bar = progressBar(0, barWidth, col);
            tail = "ready";
            break;
        case Phase::Focus:
        case Phase::Rest:
            bar = progressBar(engine_.progress(), barWidth, col);
            tail = std::to_string(static_cast<int>(engine_.progress() * 100)) + "%";
            break;
        case Phase::ExtendedFocus:
            bar = progressBar(1, barWidth, col);
            tail = "+" + formatClock(engine_.elapsedSeconds());
            break;
        case Phase::Hyperfocus:
            bar = progressBar(1, barWidth, col);
            tail = "deep";
            break;
        case Phase::Idle:
            bar = progressBar(0, barWidth, col);
            tail = "idle";
            break;
    }
    out.push_back(center(bar + " " + ansi::dim + tail + R, width));
    out.push_back("");

    if (mode_ == Mode::Input) {
        const std::string prompt = inputPurpose_ == InputPurpose::Park ? "Park a thought › " : "Task › ";
        out.push_back(std::string(ansi::bold) + prompt + R + input_ + ansi::bold + "▏" + R);
        out.push_back(std::string(ansi::dim) + "enter save · esc cancel" + R);
    } else if (mode_ == Mode::ConfirmQuit) {
        out.push_back(std::string(ansi::bold) + "Quit? The current " + phaseName(p) + " will be logged." + R);
        out.push_back(std::string(ansi::dim) + "y quit · any other key stays" + R);
    } else {
        for (const auto& line : wrap(phaseMessage(), width)) out.push_back(line);
    }

    if (!toast_.empty() && nowMs() < toastUntilMs_) {
        out.push_back("");
        bool first = true;
        for (const auto& line : wrap(toast_, width - 2)) {
            out.push_back(std::string(col) + (first ? "› " : "  ") + R + ansi::bold + line + R);
            first = false;
        }
    }
}

void App::settingsBody(std::vector<std::string>& out, int width) {
    out.push_back(std::string(ansi::bold) + "Settings" + ansi::reset);
    out.push_back("");
    const auto rows = settingRows();
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const auto& row = rows[static_cast<std::size_t>(i)];
        const bool sel = i == settingsIndex_;
        std::string value;
        if (row.flag) value = *row.flag ? "on" : "off";
        else value = std::to_string(*row.value) + " " + row.unit;
        std::string line = std::string(sel ? "▸ " : "  ") + padRight(row.label, 24) +
                           (sel ? std::string(ansi::cyan) + "‹ " + ansi::bold + value + ansi::reset + ansi::cyan + " ›"
                                : std::string(ansi::dim) + "  " + value) +
                           ansi::reset;
        out.push_back(line);
    }
    out.push_back("");
    for (const auto& l : wrapItems("↑↓ choose · ←→ change · enter toggle · esc save & close", width))
        out.push_back(std::string(ansi::dim) + l + ansi::reset);
    out.push_back(std::string(ansi::dim) + truncate(paths_.config, width) + ansi::reset);
}

std::vector<std::string> App::frame(int width) {
    std::vector<std::string> out;
    const std::string R = ansi::reset;
    const std::string right = std::string(ansi::dim) + "today " + formatDuration(todayFocusSeconds()) +
                              " · streak " + std::to_string(stats_.streakDays) + "d" + R;
    out.push_back(spread(std::string(ansi::bold) + "ADHD POMODORO" + R, right, width));
    out.push_back("");

    if (mode_ == Mode::Overlay) {
        out.push_back(std::string(ansi::bold) + overlayTitle_ + R);
        out.push_back("");
        for (const auto& l : overlayLines_) out.push_back(l);
        out.push_back("");
        out.push_back(std::string(ansi::dim) + "any key to close" + R);
        return out;
    }
    if (mode_ == Mode::Settings) {
        settingsBody(out, width);
        return out;
    }

    timerBody(out, width);
    out.push_back("");
    out.push_back(std::string(ansi::gray) + [&] {
        std::string s;
        for (int i = 0; i < width; ++i) s += "─";
        return s;
    }() + R);
    for (const auto& l : wrapItems(keyHints(), width)) out.push_back(std::string(ansi::dim) + l + R);

    std::string footer = "next focus " + m(cfg_.focusMinutes) + " · rest ≥" + m(cfg_.restMinutes);
    if (parkedCount_ > 0) footer += " · parked " + std::to_string(parkedCount_);
    if (engine_.sessionsCompleted() > 0) footer += " · done this run " + std::to_string(engine_.sessionsCompleted());
    out.push_back(std::string(ansi::dim) + footer + R);
    return out;
}

void App::render() {
    const int termW = term_->width();
    const int termH = term_->height();
    const int width = std::max(30, std::min(kMaxWidth, termW - 4));
    auto lines = frame(width);
    if (static_cast<int>(lines.size()) > termH - 1) lines.resize(static_cast<std::size_t>(std::max(1, termH - 1)));

    const std::string leftPad(static_cast<std::size_t>(std::max(0, (termW - width) / 2)), ' ');
    const int topPad = std::max(0, (termH - kBodyHeight) / 2);

    std::string out;
    if (termW != lastWidth_ || termH != lastHeight_) {
        out += "\x1b[2J";
        lastWidth_ = termW;
        lastHeight_ = termH;
        lastFrame_.clear();
    }
    out += "\x1b[H";
    for (int i = 0; i < topPad; ++i) out += "\x1b[K\r\n";
    for (const auto& l : lines) out += leftPad + l + "\x1b[K\r\n";
    out += "\x1b[J";

    // Mirror the timer into the window/tab title so it's visible from other tabs.
    const Phase p = engine_.phase();
    std::string title = "ADHD Pomodoro";
    if (p != Phase::Ready) {
        const int secs = (p == Phase::Focus || p == Phase::Rest) ? engine_.remainingSeconds()
                         : isFocusPhase(p)                     ? engine_.sessionFocusSeconds()
                                                               : engine_.elapsedSeconds();
        title = formatClock(secs) + " " + phaseLabel(p) + (engine_.paused() ? " (paused)" : "") + " · " + title;
    }
    if (title != lastTitle_) {
        out += "\x1b]0;" + title + "\a";
        lastTitle_ = title;
    }

    if (out != lastFrame_) {
        term_->write(out);
        lastFrame_ = out;
    }
}

// ---------------------------------------------------------------- main loop

int App::run() {
    refreshStats();
    {
        Terminal term;
        term_ = &term;
        while (!quit_ && !Terminal::interrupted()) {
            if (const Key k = term.readKey(100)) handleKey(k);
            engine_.tick();
            render();
        }
        engine_.stop(); // log whatever was in progress
        term_ = nullptr;
    }
    cfg_.save(paths_.config);

    std::cout << "Focused " << formatDuration(stats_.todayFocusSeconds) << " today across " << stats_.todaySessions
              << (stats_.todaySessions == 1 ? " session" : " sessions") << ".";
    if (parkedCount_ > 0) std::cout << " " << parkedCount_ << " parked thought(s) in " << paths_.parking << ".";
    std::cout << "\n";
    return 0;
}

} // namespace adhd::ui
