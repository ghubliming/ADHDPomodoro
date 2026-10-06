#pragma once
#include <cstdint>
#include <ctime>
#include <functional>
#include <string>

#include "core/Clock.h"
#include "core/Config.h"
#include "core/Phase.h"

namespace adhd {

// One finished phase, as written to the journal.
struct PhaseRecord {
    std::int64_t sessionId = 0; // groups a focus session with the rest/idle that follow it
    std::time_t startedAt = 0;
    Phase phase = Phase::Ready;
    int seconds = 0;
    Rating rating = Rating::None;
    std::string task;
};

// The Pomodoro state machine. Pure logic: no I/O, no threads. Call tick()
// regularly; react to what happened through the listener.
class Engine {
public:
    struct Event {
        enum class Type {
            PhaseChanged,   // from -> to; `automatic` when a timer ran out
            PhaseCompleted, // `record` is ready to be logged
            Nudge,          // gentle reminder; `nudgeCount` reminders so far in this phase
            FocusAdjusted,  // adaptive focus target moved old -> new
        };
        Type type = Type::PhaseChanged;
        Phase from = Phase::Ready;
        Phase to = Phase::Ready;
        bool automatic = false;
        PhaseRecord record;
        int nudgeCount = 0;
        int oldFocusMinutes = 0;
        int newFocusMinutes = 0;
        Rating rating = Rating::None;
    };
    using Listener = std::function<void(const Event&)>;

    Engine(Config& config, const Clock& clock);
    void setListener(Listener listener) { listener_ = std::move(listener); }

    // Actions. Each returns false (and does nothing) when not valid right now.
    bool startFocus();         // Ready / Rest / Idle -> Focus
    bool startTinyFocus();     // same, but only tinyStartMinutes long and never adapts
    bool togglePause();        // any phase except Ready
    bool rate(Rating rating);  // Focus / Extended / Hyperfocus -> Rest (Rating::None = no rating)
    bool enterHyperfocus();    // Focus / Extended -> Hyperfocus
    bool stop();               // anything -> Ready
    void tick();               // advance automatic transitions and nudges

    Phase phase() const { return phase_; }
    bool paused() const { return paused_; }
    bool isTinyStart() const { return tiny_; }
    int elapsedSeconds() const;          // in the current phase
    int remainingSeconds() const;        // countdown phases (Focus, Rest); 0 otherwise
    int targetSeconds() const;           // Focus / Rest target; 0 otherwise
    double progress() const;             // 0..1 for countdown phases, -1 otherwise
    int sessionFocusSeconds() const;     // focus + extended + hyperfocus so far this session
    int sessionsCompleted() const { return sessionsCompleted_; }
    Rating lastRating() const { return lastRating_; }

    const std::string& task() const { return task_; }
    void setTask(std::string task) { task_ = std::move(task); }

    // Rest earned after focusing `focusSeconds`: max(rest, focus * rest%) capped
    // at max_rest, rounded to whole minutes.
    int restSecondsFor(int focusSeconds) const;

private:
    std::int64_t elapsedMs() const;
    bool beginFocus(int minutes, bool tiny);
    void complete(int seconds, Rating rating);
    void enter(Phase to, std::int64_t startMs, std::time_t startWall, bool automatic);
    void adapt(Rating rating, bool reachedTarget);
    void emit(const Event& e) const;

    Config& cfg_;
    const Clock& clock_;
    Listener listener_;

    Phase phase_ = Phase::Ready;
    std::int64_t phaseStartMs_ = 0;
    std::time_t phaseStartWall_ = 0;
    std::int64_t pausedMs_ = 0;
    std::int64_t pauseStartMs_ = 0;
    bool paused_ = false;
    int nudgesSent_ = 0;

    int focusTargetSec_ = 0;
    int restTargetSec_ = 0;
    bool tiny_ = false;
    int sessionFocusDoneSec_ = 0; // finished focus-type phases in this session
    std::int64_t sessionId_ = 0;
    int sessionsCompleted_ = 0;
    Rating lastRating_ = Rating::None;
    std::string task_;
};

} // namespace adhd
