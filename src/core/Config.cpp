#include "core/Config.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sstream>

namespace adhd {

namespace {

std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

bool parseInt(const std::string& s, int& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    errno = 0;
    const long v = std::strtol(s.c_str(), &end, 10);
    if (errno != 0 || end == s.c_str() || *end != '\0') return false;
    out = static_cast<int>(std::clamp(v, -100000L, 100000L));
    return true;
}

bool parseBool(const std::string& s, bool& out) {
    std::string v;
    for (char c : s) v += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (v == "true" || v == "yes" || v == "on" || v == "1") { out = true; return true; }
    if (v == "false" || v == "no" || v == "off" || v == "0") { out = false; return true; }
    return false;
}

struct IntKey { const char* key; int Config::*field; const char* comment; };
struct BoolKey { const char* key; bool Config::*field; const char* comment; };

const IntKey kIntKeys[] = {
    {"focus_minutes", &Config::focusMinutes, "Focus target. Adapts to your ratings when adaptive = true."},
    {"rest_minutes", &Config::restMinutes, "Minimum rest."},
    {"tiny_start_minutes", &Config::tinyStartMinutes, "Length of a 'tiny start' (key m) for when starting feels impossible."},
    {"adapt_step_minutes", &Config::adaptStepMinutes, "How much one good/bad rating moves the focus target."},
    {"min_focus_minutes", &Config::minFocusMinutes, "Adaptive lower bound."},
    {"max_focus_minutes", &Config::maxFocusMinutes, "Adaptive upper bound."},
    {"rest_percent", &Config::restPercent, "Proportional rest: a break is at least this % of the time you focused."},
    {"max_rest_minutes", &Config::maxRestMinutes, "Cap for proportional rest."},
    {"nudge_minutes", &Config::nudgeMinutes, "Gentle reminder interval in extended focus and idle (0 = off)."},
    {"hyperfocus_check_minutes", &Config::hyperfocusCheckMinutes, "Body check-in interval during hyperfocus (0 = off)."},
};

const BoolKey kBoolKeys[] = {
    {"adaptive", &Config::adaptive, "Good sessions grow the focus target, bad ones shrink it."},
    {"bell", &Config::bell, "Terminal bell when a timer ends and on nudges."},
    {"notify", &Config::notify, "Desktop notifications (Linux: notify-send, macOS: osascript)."},
};

} // namespace

void Config::clamp() {
    focusMinutes = std::clamp(focusMinutes, 1, 240);
    restMinutes = std::clamp(restMinutes, 1, 120);
    tinyStartMinutes = std::clamp(tinyStartMinutes, 1, 60);
    adaptStepMinutes = std::clamp(adaptStepMinutes, 0, 30);
    minFocusMinutes = std::clamp(minFocusMinutes, 1, 240);
    maxFocusMinutes = std::clamp(maxFocusMinutes, minFocusMinutes, 240);
    restPercent = std::clamp(restPercent, 0, 100);
    maxRestMinutes = std::clamp(maxRestMinutes, restMinutes, 240);
    nudgeMinutes = std::clamp(nudgeMinutes, 0, 120);
    hyperfocusCheckMinutes = std::clamp(hyperfocusCheckMinutes, 0, 240);
}

Config Config::parse(std::istream& in) {
    Config c;
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    // Legacy v1 settings.txt: just "<focus> <rest>".
    {
        std::istringstream legacy(content);
        int focus = 0, rest = 0;
        std::string extra;
        if (legacy >> focus >> rest && !(legacy >> extra)) {
            c.focusMinutes = focus;
            c.restMinutes = rest;
            c.clamp();
            return c;
        }
    }

    std::istringstream lines(content);
    std::string line;
    while (std::getline(lines, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = trim(line.substr(0, eq));
        const std::string value = trim(line.substr(eq + 1));

        if (key.rfind("hook.", 0) == 0) {
            if (!value.empty()) c.hooks[key.substr(5)] = value;
            continue;
        }
        for (const auto& k : kIntKeys) {
            if (key == k.key) parseInt(value, c.*(k.field));
        }
        for (const auto& k : kBoolKeys) {
            if (key == k.key) parseBool(value, c.*(k.field));
        }
    }
    c.clamp();
    return c;
}

void Config::write(std::ostream& out) const {
    out << "# ADHD Pomodoro settings. Edit freely; lines starting with # are comments.\n"
        << "# Durations are in minutes.\n\n";
    for (const auto& k : kIntKeys) {
        out << "# " << k.comment << "\n" << k.key << " = " << this->*(k.field) << "\n\n";
    }
    for (const auto& k : kBoolKeys) {
        out << "# " << k.comment << "\n" << k.key << " = " << (this->*(k.field) ? "true" : "false") << "\n\n";
    }
    out << "# Hooks: shell commands run in the background when a phase starts.\n"
        << "# Phases: focus, extended, hyperfocus, rest, idle, ready. {phase} expands to the phase name.\n"
        << "# Calm music while focusing, something loud for hyperfocus, happy tunes on breaks:\n"
        << "#   hook.focus      = playerctl open spotify:playlist:<calm-lofi>\n"
        << "#   hook.hyperfocus = playerctl open spotify:playlist:<phonk>\n"
        << "#   hook.rest       = playerctl open spotify:playlist:<happy>\n";
    for (const auto& [phase, cmd] : hooks) out << "hook." << phase << " = " << cmd << "\n";
}

Config Config::load(const std::string& path, bool* existed) {
    std::ifstream in(path);
    if (existed) *existed = static_cast<bool>(in);
    if (!in) return Config{};
    return parse(in);
}

bool Config::save(const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;
    write(out);
    return static_cast<bool>(out);
}

} // namespace adhd
