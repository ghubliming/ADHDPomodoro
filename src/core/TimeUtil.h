#pragma once
#include <cstdint>
#include <ctime>
#include <string>

namespace adhd {

std::string formatClock(int seconds);    // "05:07", or "1:02:03" past an hour
std::string formatDuration(int seconds); // "1h 25m", "25m", "40s"

std::tm localTm(std::time_t t);
std::string formatLocalTime(std::time_t t, const char* fmt = "%Y-%m-%d %H:%M:%S");
// Parses "YYYY-MM-DD HH:MM:SS" as local time.
bool parseLocalTime(const std::string& s, std::time_t& out);

// Day numbers: days since 1970-01-01 in the proleptic Gregorian calendar.
std::int64_t daysFromCivil(int y, unsigned m, unsigned d);
std::int64_t localDayNumber(std::time_t t);
int localHour(std::time_t t);
const char* weekdayShort(std::int64_t dayNumber); // "Mon".."Sun"

} // namespace adhd
