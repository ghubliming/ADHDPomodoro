#include "core/Phase.h"

namespace adhd {

namespace {
constexpr Phase kAllPhases[] = {Phase::Ready, Phase::Focus, Phase::ExtendedFocus,
                                Phase::Hyperfocus, Phase::Rest, Phase::Idle};
constexpr Rating kAllRatings[] = {Rating::None, Rating::Good, Rating::Okay, Rating::Bad};
} // namespace

const char* phaseName(Phase p) {
    switch (p) {
        case Phase::Ready: return "ready";
        case Phase::Focus: return "focus";
        case Phase::ExtendedFocus: return "extended";
        case Phase::Hyperfocus: return "hyperfocus";
        case Phase::Rest: return "rest";
        case Phase::Idle: return "idle";
    }
    return "unknown";
}

const char* phaseLabel(Phase p) {
    switch (p) {
        case Phase::Ready: return "READY";
        case Phase::Focus: return "FOCUS";
        case Phase::ExtendedFocus: return "EXTENDED FOCUS";
        case Phase::Hyperfocus: return "HYPERFOCUS";
        case Phase::Rest: return "REST";
        case Phase::Idle: return "IDLE";
    }
    return "UNKNOWN";
}

const char* ratingName(Rating r) {
    switch (r) {
        case Rating::None: return "";
        case Rating::Good: return "good";
        case Rating::Okay: return "okay";
        case Rating::Bad: return "bad";
    }
    return "";
}

bool parsePhase(const std::string& s, Phase& out) {
    for (Phase p : kAllPhases) {
        if (s == phaseName(p)) {
            out = p;
            return true;
        }
    }
    return false;
}

bool parseRating(const std::string& s, Rating& out) {
    for (Rating r : kAllRatings) {
        if (s == ratingName(r)) {
            out = r;
            return true;
        }
    }
    return false;
}

bool isFocusPhase(Phase p) {
    return p == Phase::Focus || p == Phase::ExtendedFocus || p == Phase::Hyperfocus;
}

} // namespace adhd
