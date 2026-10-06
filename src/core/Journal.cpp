#include "core/Journal.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "core/TimeUtil.h"

namespace adhd {

std::string csvEscape(const std::string& field) {
    if (field.find_first_of(",\"\r\n") == std::string::npos) return field;
    std::string out = "\"";
    for (char c : field) {
        if (c == '\r' || c == '\n') c = ' ';
        if (c == '"') out += '"';
        out += c;
    }
    return out + "\"";
}

std::vector<std::string> csvSplit(const std::string& line) {
    std::vector<std::string> fields(1);
    bool quoted = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (quoted) {
            if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') {
                fields.back() += '"';
                ++i;
            } else if (c == '"') {
                quoted = false;
            } else {
                fields.back() += c;
            }
        } else if (c == '"') {
            quoted = true;
        } else if (c == ',') {
            fields.emplace_back();
        } else if (c != '\r') {
            fields.back() += c;
        }
    }
    return fields;
}

const char* Journal::header() { return "session_id,started_at,phase,seconds,rating,task"; }

std::string Journal::toCsvLine(const PhaseRecord& r) {
    return std::to_string(r.sessionId) + "," + formatLocalTime(r.startedAt) + "," + phaseName(r.phase) + "," +
           std::to_string(r.seconds) + "," + ratingName(r.rating) + "," + csvEscape(r.task);
}

bool Journal::fromCsvLine(const std::string& line, PhaseRecord& out) {
    const auto f = csvSplit(line);
    if (f.size() < 5) return false;
    PhaseRecord r;
    char* end = nullptr;
    r.sessionId = std::strtoll(f[0].c_str(), &end, 10);
    if (f[0].empty() || *end != '\0') return false;
    if (!parseLocalTime(f[1], r.startedAt)) return false;
    if (!parsePhase(f[2], r.phase)) return false;
    r.seconds = static_cast<int>(std::strtol(f[3].c_str(), &end, 10));
    if (f[3].empty() || *end != '\0' || r.seconds < 0) return false;
    if (!parseRating(f[4], r.rating)) return false;
    if (f.size() > 5) r.task = f[5];
    out = std::move(r);
    return true;
}

bool Journal::append(const PhaseRecord& record) const {
    std::error_code ec;
    const bool fresh = !std::filesystem::exists(path_, ec) || std::filesystem::file_size(path_, ec) == 0;
    std::ofstream out(path_, std::ios::app);
    if (!out) return false;
    if (fresh) out << header() << "\n";
    out << toCsvLine(record) << "\n";
    return static_cast<bool>(out);
}

std::vector<PhaseRecord> Journal::readAll(int* skippedLines) const {
    std::vector<PhaseRecord> records;
    int skipped = 0;
    std::ifstream in(path_);
    std::string line;
    bool first = true;
    while (std::getline(in, line)) {
        if (first) {
            first = false;
            if (line.rfind("session_id", 0) == 0) continue;
        }
        if (line.empty() || line == "\r") continue;
        PhaseRecord r;
        if (fromCsvLine(line, r)) records.push_back(std::move(r));
        else ++skipped;
    }
    if (skippedLines) *skippedLines = skipped;
    return records;
}

} // namespace adhd
