#pragma once
#include <string>
#include <vector>

#include "core/Stats.h"

namespace adhd::ui {

// Human-readable stats report, shared by `adhd-pomodoro stats` and the in-app overlay.
std::vector<std::string> statsReport(const Stats& stats, bool color);

} // namespace adhd::ui
