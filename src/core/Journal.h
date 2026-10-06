#pragma once
#include <string>
#include <vector>

#include "core/Engine.h"

namespace adhd {

// Append-only CSV log of finished phases:
//   session_id,started_at,phase,seconds,rating,task
// Plain CSV on purpose: open it in a spreadsheet, or analyse it with anything.
class Journal {
public:
    explicit Journal(std::string path) : path_(std::move(path)) {}

    bool append(const PhaseRecord& record) const;
    std::vector<PhaseRecord> readAll(int* skippedLines = nullptr) const;
    const std::string& path() const { return path_; }

    static const char* header();
    static std::string toCsvLine(const PhaseRecord& record);
    static bool fromCsvLine(const std::string& line, PhaseRecord& out);

private:
    std::string path_;
};

std::string csvEscape(const std::string& field);
std::vector<std::string> csvSplit(const std::string& line);

} // namespace adhd
