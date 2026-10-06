#pragma once
#include <string>

namespace adhd {

// The ADHD-flavoured Pomodoro cycle:
//
//   Ready ─▶ Focus ──(timer ends)──▶ ExtendedFocus ──(rate g/o/b)──▶ Rest ──(timer ends)──▶ Idle
//              │                        │                             ▲                    │
//              └──────(h)──▶ Hyperfocus ┘──────────(rate / r)─────────┘   ◀──(s)── start ──┘
//
// Focus counts down. When it hits zero you are *not* interrupted: the timer keeps
// counting up in ExtendedFocus until you surface and rate the session.
enum class Phase { Ready, Focus, ExtendedFocus, Hyperfocus, Rest, Idle };

enum class Rating { None, Good, Okay, Bad };

const char* phaseName(Phase p);   // stable lowercase id used in logs/config ("extended")
const char* phaseLabel(Phase p);  // human label ("EXTENDED FOCUS")
const char* ratingName(Rating r); // "good" / "okay" / "bad" / ""

bool parsePhase(const std::string& s, Phase& out);
bool parseRating(const std::string& s, Rating& out);

// Focus, ExtendedFocus and Hyperfocus all count as time spent focusing.
bool isFocusPhase(Phase p);

} // namespace adhd
