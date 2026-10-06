#include "core/Paths.h"

#include <cstdlib>
#include <filesystem>

namespace adhd {

namespace fs = std::filesystem;

namespace {
std::string env(const char* name) {
    const char* v = std::getenv(name);
    return v ? std::string(v) : std::string();
}
} // namespace

Paths Paths::in(const std::string& dir) {
    Paths p;
    p.dir = dir;
    p.config = (fs::path(dir) / "settings.conf").string();
    p.journal = (fs::path(dir) / "sessions.csv").string();
    p.parking = (fs::path(dir) / "parked.md").string();
    return p;
}

Paths Paths::resolve(const std::string& explicitDir) {
    if (!explicitDir.empty()) return in(explicitDir);
    if (auto home = env("ADHD_POMODORO_HOME"); !home.empty()) return in(home);
#ifdef _WIN32
    if (auto appData = env("APPDATA"); !appData.empty()) return in((fs::path(appData) / "ADHDPomodoro").string());
#endif
    if (auto xdg = env("XDG_DATA_HOME"); !xdg.empty()) return in((fs::path(xdg) / "adhd-pomodoro").string());
    if (auto home = env("HOME"); !home.empty())
        return in((fs::path(home) / ".local" / "share" / "adhd-pomodoro").string());
    return in(".");
}

bool Paths::ensureDir() const {
    std::error_code ec;
    fs::create_directories(dir, ec);
    return fs::is_directory(dir, ec);
}

} // namespace adhd
