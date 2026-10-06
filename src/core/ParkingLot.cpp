#include "core/ParkingLot.h"

#include <filesystem>
#include <fstream>

#include "core/TimeUtil.h"

namespace adhd {

namespace {
constexpr const char* kOpen = "- [ ] ";
}

bool ParkingLot::add(const std::string& thought, std::time_t when) const {
    std::string text;
    for (char c : thought) text += (c == '\n' || c == '\r') ? ' ' : c;
    if (text.find_first_not_of(' ') == std::string::npos) return false;

    std::error_code ec;
    const bool fresh = !std::filesystem::exists(path_, ec) || std::filesystem::file_size(path_, ec) == 0;
    std::ofstream out(path_, std::ios::app);
    if (!out) return false;
    if (fresh) out << "# Parked thoughts\n\nDeal with these on a break. Tick them off with [x].\n\n";
    out << kOpen << text << "  _(" << formatLocalTime(when, "%Y-%m-%d %H:%M") << ")_\n";
    return static_cast<bool>(out);
}

std::vector<std::string> ParkingLot::openItems() const {
    std::vector<std::string> items;
    std::ifstream in(path_);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind(kOpen, 0) != 0) continue;
        std::string item = line.substr(6);
        const auto stamp = item.rfind("  _(");
        if (stamp != std::string::npos) item.erase(stamp);
        items.push_back(item);
    }
    return items;
}

} // namespace adhd
