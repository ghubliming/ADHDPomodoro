#pragma once
#include <ctime>
#include <string>
#include <vector>

namespace adhd {

// Distraction capture. When a thought pops up mid-focus ("reply to Sam!",
// "what was that movie?"), park it here in two seconds instead of acting on it,
// and deal with it on your break. Stored as a Markdown checklist, so you can
// tick items off in any editor.
class ParkingLot {
public:
    explicit ParkingLot(std::string path) : path_(std::move(path)) {}

    bool add(const std::string& thought, std::time_t when) const;
    std::vector<std::string> openItems() const; // unchecked "- [ ]" lines, oldest first
    const std::string& path() const { return path_; }

private:
    std::string path_;
};

} // namespace adhd
