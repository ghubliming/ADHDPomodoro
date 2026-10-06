#include "core/Engine.h"

#include <algorithm>

namespace adhd {

Engine::Engine(Config& config, const Clock& clock) : cfg_(config), clock_(clock) {}

std::int64_t Engine::elapsedMs() const {
    if (phase_ == Phase::Ready) return 0;
    const std::int64_t end = paused_ ? pauseStartMs_ : clock_.monotonicMs();
    return std::max<std::int64_t>(0, end - phaseStartMs_ - pausedMs_);
}

int Engine::elapsedSeconds() const { return static_cast<int>(elapsedMs() / 1000); }

int Engine::targetSeconds() const {
    switch (phase_) {
        case Phase::Focus: return focusTargetSec_;
        case Phase::Rest: return restTargetSec_;
        default: return 0;
    }
}

int Engine::remainingSeconds() const {
    const int target = targetSeconds();
    if (target == 0) return 0;
    const std::int64_t left = static_cast<std::int64_t>(target) * 1000 - elapsedMs();
    return left <= 0 ? 0 : static_cast<int>((left + 999) / 1000); // round up: shows 15:00 at the start
}

double Engine::progress() const {
    const int target = targetSeconds();
    if (target == 0) return -1.0;
    return std::min(1.0, static_cast<double>(elapsedMs()) / (target * 1000.0));
}

int Engine::sessionFocusSeconds() const {
    return sessionFocusDoneSec_ + (isFocusPhase(phase_) ? elapsedSeconds() : 0);
}

int Engine::restSecondsFor(int focusSeconds) const {
    const int base = cfg_.restMinutes * 60;
    int proportional = focusSeconds * cfg_.restPercent / 100;
    proportional = (proportional + 30) / 60 * 60;
    const int cap = std::max(base, cfg_.maxRestMinutes * 60);
    return std::min(std::max(base, proportional), cap);
}

void Engine::emit(const Event& e) const {
    if (listener_) listener_(e);
}

void Engine::complete(int seconds, Rating rating) {
    if (phase_ == Phase::Ready) return;
    if (isFocusPhase(phase_)) sessionFocusDoneSec_ += seconds;
    Event e;
    e.type = Event::Type::PhaseCompleted;
    e.from = phase_;
    e.record = PhaseRecord{sessionId_, phaseStartWall_, phase_, seconds, rating, task_};
    emit(e);
}

void Engine::enter(Phase to, std::int64_t startMs, std::time_t startWall, bool automatic) {
    const Phase from = phase_;
    phase_ = to;
    phaseStartMs_ = startMs;
    phaseStartWall_ = startWall;
    pausedMs_ = 0;
    paused_ = false;
    nudgesSent_ = 0;
    Event e;
    e.type = Event::Type::PhaseChanged;
    e.from = from;
    e.to = to;
    e.automatic = automatic;
    emit(e);
}

bool Engine::beginFocus(int minutes, bool tiny) {
    if (phase_ != Phase::Ready && phase_ != Phase::Rest && phase_ != Phase::Idle) return false;
    complete(elapsedSeconds(), Rating::None);
    const std::time_t wall = clock_.wallTime();
    sessionId_ = std::max<std::int64_t>(static_cast<std::int64_t>(wall), sessionId_ + 1);
    sessionFocusDoneSec_ = 0;
    focusTargetSec_ = minutes * 60;
    tiny_ = tiny;
    lastRating_ = Rating::None;
    enter(Phase::Focus, clock_.monotonicMs(), wall, false);
    return true;
}

bool Engine::startFocus() { return beginFocus(cfg_.focusMinutes, false); }

bool Engine::startTinyFocus() { return beginFocus(cfg_.tinyStartMinutes, true); }

bool Engine::togglePause() {
    if (phase_ == Phase::Ready) return false;
    const std::int64_t now = clock_.monotonicMs();
    if (paused_) {
        pausedMs_ += now - pauseStartMs_;
        paused_ = false;
    } else {
        pauseStartMs_ = now;
        paused_ = true;
    }
    return true;
}

void Engine::adapt(Rating rating, bool reachedTarget) {
    if (!cfg_.adaptive || tiny_ || cfg_.adaptStepMinutes == 0) return;
    const int old = cfg_.focusMinutes;
    int next = old;
    // Only grow after a session that actually outlasted its timer; a good-but-short
    // session says nothing about being able to go longer.
    if (rating == Rating::Good && reachedTarget && old < cfg_.maxFocusMinutes)
        next = std::min(old + cfg_.adaptStepMinutes, cfg_.maxFocusMinutes);
    else if (rating == Rating::Bad && old > cfg_.minFocusMinutes)
        next = std::max(old - cfg_.adaptStepMinutes, cfg_.minFocusMinutes);
    if (next == old) return;
    cfg_.focusMinutes = next;
    Event e;
    e.type = Event::Type::FocusAdjusted;
    e.oldFocusMinutes = old;
    e.newFocusMinutes = next;
    e.rating = rating;
    emit(e);
}

bool Engine::rate(Rating rating) {
    if (!isFocusPhase(phase_)) return false;
    complete(elapsedSeconds(), rating);
    adapt(rating, sessionFocusDoneSec_ >= focusTargetSec_);
    restTargetSec_ = restSecondsFor(sessionFocusDoneSec_);
    ++sessionsCompleted_;
    lastRating_ = rating;
    enter(Phase::Rest, clock_.monotonicMs(), clock_.wallTime(), false);
    return true;
}

bool Engine::enterHyperfocus() {
    if (phase_ != Phase::Focus && phase_ != Phase::ExtendedFocus) return false;
    complete(elapsedSeconds(), Rating::None);
    enter(Phase::Hyperfocus, clock_.monotonicMs(), clock_.wallTime(), false);
    return true;
}

bool Engine::stop() {
    if (phase_ == Phase::Ready) return false;
    complete(elapsedSeconds(), Rating::None);
    enter(Phase::Ready, clock_.monotonicMs(), clock_.wallTime(), false);
    return true;
}

void Engine::tick() {
    if (phase_ == Phase::Ready || paused_) return;
    const std::int64_t ms = elapsedMs();

    if (phase_ == Phase::Focus || phase_ == Phase::Rest) {
        const int target = targetSeconds();
        const std::int64_t targetMs = static_cast<std::int64_t>(target) * 1000;
        if (ms < targetMs) return;
        // The next phase starts exactly when the timer ran out, so time spent
        // asleep/suspended past the target is still counted.
        const std::int64_t overshoot = ms - targetMs;
        const Phase next = phase_ == Phase::Focus ? Phase::ExtendedFocus : Phase::Idle;
        complete(target, Rating::None);
        enter(next, clock_.monotonicMs() - overshoot,
              clock_.wallTime() - static_cast<std::time_t>(overshoot / 1000), true);
        tick(); // a long overshoot may already be due a nudge
        return;
    }

    const int interval = phase_ == Phase::Hyperfocus ? cfg_.hyperfocusCheckMinutes : cfg_.nudgeMinutes;
    if (interval <= 0) return;
    const int due = static_cast<int>(ms / (static_cast<std::int64_t>(interval) * 60000));
    if (due > nudgesSent_) {
        nudgesSent_ = due;
        Event e;
        e.type = Event::Type::Nudge;
        e.from = phase_;
        e.to = phase_;
        e.nudgeCount = due;
        emit(e);
    }
}

} // namespace adhd
