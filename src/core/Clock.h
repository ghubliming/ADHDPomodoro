#pragma once
#include <chrono>
#include <cstdint>
#include <ctime>

namespace adhd {

// Time source for the engine. Phase lengths use a monotonic clock (immune to
// NTP/DST jumps); log timestamps use wall-clock time.
class Clock {
public:
    virtual ~Clock() = default;
    virtual std::int64_t monotonicMs() const = 0;
    virtual std::time_t wallTime() const = 0;
};

class SystemClock : public Clock {
public:
    std::int64_t monotonicMs() const override {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }
    std::time_t wallTime() const override { return std::time(nullptr); }
};

// Hand-cranked clock for tests.
class ManualClock : public Clock {
public:
    explicit ManualClock(std::time_t wallStart = 1767225600) : wallStart_(wallStart) {}
    std::int64_t monotonicMs() const override { return ms_; }
    std::time_t wallTime() const override { return wallStart_ + static_cast<std::time_t>(ms_ / 1000); }
    void advanceSeconds(std::int64_t s) { ms_ += s * 1000; }
    void advanceMs(std::int64_t ms) { ms_ += ms; }

private:
    std::int64_t ms_ = 0;
    std::time_t wallStart_;
};

} // namespace adhd
