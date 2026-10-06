#include "core/TimeUtil.h"

#include <cstdio>
#include <iomanip>
#include <sstream>

namespace adhd {

std::string formatClock(int seconds) {
    if (seconds < 0) seconds = 0;
    const int h = seconds / 3600;
    const int m = (seconds / 60) % 60;
    const int s = seconds % 60;
    std::ostringstream oss;
    oss << std::setfill('0');
    if (h > 0) oss << h << ':' << std::setw(2) << m;
    else oss << std::setw(2) << m;
    oss << ':' << std::setw(2) << s;
    return oss.str();
}

std::string formatDuration(int seconds) {
    if (seconds < 0) seconds = 0;
    if (seconds < 60) return std::to_string(seconds) + "s";
    const int h = seconds / 3600;
    const int m = (seconds / 60) % 60;
    if (h == 0) return std::to_string(m) + "m";
    if (m == 0) return std::to_string(h) + "h";
    return std::to_string(h) + "h " + std::to_string(m) + "m";
}

std::tm localTm(std::time_t t) {
    std::tm out{};
#ifdef _WIN32
    localtime_s(&out, &t);
#else
    localtime_r(&t, &out);
#endif
    return out;
}

std::string formatLocalTime(std::time_t t, const char* fmt) {
    const std::tm tm = localTm(t);
    std::ostringstream oss;
    oss << std::put_time(&tm, fmt);
    return oss.str();
}

bool parseLocalTime(const std::string& s, std::time_t& out) {
    std::tm tm{};
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, sec = 0;
    if (std::sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &sec) != 6) return false;
    if (mo < 1 || mo > 12 || d < 1 || d > 31 || h < 0 || h > 23 || mi < 0 || mi > 59 || sec < 0 || sec > 60)
        return false;
    tm.tm_year = y - 1900;
    tm.tm_mon = mo - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = mi;
    tm.tm_sec = sec;
    tm.tm_isdst = -1;
    const std::time_t t = std::mktime(&tm);
    if (t == static_cast<std::time_t>(-1)) return false;
    out = t;
    return true;
}

// Howard Hinnant's days_from_civil.
std::int64_t daysFromCivil(int y, unsigned m, unsigned d) {
    if (m <= 2) --y;
    const std::int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned mp = m > 2 ? m - 3 : m + 9; // March-based month
    const unsigned doy = (153 * mp + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<std::int64_t>(doe) - 719468;
}

std::int64_t localDayNumber(std::time_t t) {
    const std::tm tm = localTm(t);
    return daysFromCivil(tm.tm_year + 1900, static_cast<unsigned>(tm.tm_mon + 1),
                         static_cast<unsigned>(tm.tm_mday));
}

int localHour(std::time_t t) { return localTm(t).tm_hour; }

const char* weekdayShort(std::int64_t dayNumber) {
    static const char* names[] = {"Thu", "Fri", "Sat", "Sun", "Mon", "Tue", "Wed"}; // day 0 was a Thursday
    std::int64_t i = dayNumber % 7;
    if (i < 0) i += 7;
    return names[i];
}

} // namespace adhd
