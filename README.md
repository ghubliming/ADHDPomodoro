# ADHD Pomodoro

A focus timer that works the way ADHD attention actually works: in waves, not 25-minute blocks.

```
        ADHD POMODORO                               today 1h 25m · streak 3d
                             █   █   █   █████  ███
                            ██   █   █ ▪ █     █   █
                             █   █████   ████   ████
                             █       █ ▪     █     █
                            ███      █   ████   ███
                           ● FOCUS  · write chapter 2
            ███████████████████████████▍───────────────────────── 47%
        A thought pops up? Press d to park it, then carry on.
        › Parked. Back to it! (2 waiting for your break)
        ────────────────────────────────────────────────────────────────
        space pause · g/o/b end+rate · h hyperfocus · d park · t task
        x stop
        next focus 17m · rest ≥5m · parked 2
```

Classic Pomodoro rings a bell at minute 25 and kills your flow right when you finally got into it.
This one doesn't:

| Phase | What happens |
|---|---|
| **Focus** | Counts down from your focus target. |
| **Extended Focus** | The timer ran out, but **you are not interrupted**. It keeps counting up until you surface and rate the session. |
| **Hyperfocus** | Opt in with `h` for deep sessions. Gentle body check-ins every 30 min (water, posture, is this still the right task?). |
| **Rest** | Earned in proportion: at least `rest_minutes`, or 20% of the time you focused if that's longer (capped). |
| **Idle** | The break is over and the timer counts how long restarting takes, with nudges toward a *tiny start*. |

## What makes it ADHD-friendly

- **Adaptive focus length.** Rate each session `g`ood / `o`kay / `b`ad. A good session that outlasted its timer
  makes the next one 2 min longer, and a bad one makes it 2 min shorter. Your focus length drifts toward what your brain
  can actually do today instead of what a productivity book says.
- **Parking lot.** Mid-focus thought ("reply to Sam!")? Press `d`, type it, hit Enter, and you're back in under
  five seconds. Thoughts land in a Markdown checklist (`parked.md`) for your break.
- **Tiny start.** Can't get going? `m` starts a 5-minute session. If it flows, Extended Focus just keeps going.
  (Tiny starts never shrink your adaptive target.)
- **No guilt.** Stopping early still logs what you did. Bad sessions are "data, not failure".
- **Hard to lose track of.** The timer shows in the terminal tab title, the bell rings when a timer ends,
  and there are optional desktop notifications plus gentle reminders when you've drifted into idle.
- **Hooks for other apps.** Run any command when a phase starts. For example, the old playlist idea:
  calm music for focus, phonk for hyperfocus, happy tunes on breaks (see [Hooks](#hooks)).
- **Stats that matter.** Today's total, streak, a 7-day sparkline, rating mix, your best hour of the day,
  and how long it usually takes you to restart after a break.

## Build

Needs only a C++17 compiler and CMake ≥ 3.16. Works on Linux, macOS and Windows.

```bash
git clone https://github.com/ghubliming/ADHDPomodoro.git
cd ADHDPomodoro
cmake -S . -B build -DBUILD_CLI_ONLY=ON
cmake --build build --config Release
ctest --test-dir build -C Release      # optional: run the unit tests
```

The binary is `build/adhd-pomodoro` (on Windows `build\Release\adhd-pomodoro.exe`).

## Use

```bash
adhd-pomodoro                      # run the timer
adhd-pomodoro -t "write chapter 2" # ...with a task name
adhd-pomodoro stats                # your stats (--days 14 for a longer chart)
adhd-pomodoro parked               # list parked thoughts
adhd-pomodoro where                # where settings and logs live
adhd-pomodoro --focus 20 --rest 5  # change the defaults (saved)
```

### Keys

| Key | Action | Key | Action |
|---|---|---|---|
| `s` / `space` | start focus | `m` | tiny start (5 min) |
| `space` / `p` | pause / resume | `x` | stop (still logged) |
| `g` `o` `b` | end session and rate it | `r` | end session, no rating |
| `h` | hyperfocus | `d` | park a distracting thought |
| `t` | name the task | `v` | review parked thoughts |
| `i` | stats | `c` | settings |
| `?` | help | `q` | quit (asks first if a timer is running) |

## Your data

Everything is plain text in one folder (`adhd-pomodoro where` shows it):
`~/.local/share/adhd-pomodoro` on Linux and macOS, `%APPDATA%\ADHDPomodoro` on Windows.
Override with `--data-dir DIR` or `ADHD_POMODORO_HOME`.

- `settings.conf`: commented `key = value` settings. Edit it by hand or press `c` in the app.
  A v1 `settings.txt` in the current directory is imported automatically on first run.
- `sessions.csv`: one row per finished phase (`session_id,started_at,phase,seconds,rating,task`).
  Open it in a spreadsheet and analyse away.
- `parked.md`: your parked thoughts as a checklist. Tick items off with `[x]`.

### Hooks

Add lines like these to `settings.conf`. Each command runs in the background when that phase starts,
and `{phase}` expands to the phase name.

```ini
hook.focus      = playerctl open spotify:playlist:<calm-lofi>
hook.hyperfocus = playerctl open spotify:playlist:<phonk>
hook.rest       = playerctl open spotify:playlist:<happy>
hook.idle       = notify-send "Break's over"
```

Phases: `focus`, `extended`, `hyperfocus`, `rest`, `idle`, `ready`.

## Code tour

```
src/core/    platform-free logic, no dependencies, fully unit-tested
  Engine     the state machine; time comes from an injectable Clock so tests can fast-forward
  Config     settings file (current + legacy v1 format)
  Journal    CSV session log        Stats       aggregates + sparkline
  ParkingLot distraction capture    Paths       per-OS data directory
src/cli/     the terminal app: raw input, flicker-free rendering, big digits, overlays
src/gui/     the original SFML always-on-top widget (Windows, v1 logic)
tests/       a tiny self-contained test runner (no framework needed)
```

The engine never sleeps and never touches the terminal. You feed it a clock and call `tick()`, and it reports
what happened through events (`PhaseChanged`, `PhaseCompleted`, `Nudge`, `FocusAdjusted`). That's what
makes things like "laptop slept for 40 minutes mid-focus" testable in a millisecond.

### The original SFML widget

`src/gui` still holds the v1 always-on-top window. It builds when SFML 2.5 is found (or at `C:/SFML` on Windows)
and `BUILD_CLI_ONLY` is off. It doesn't use the new engine yet, which makes it the obvious next project.

## Ideas for later

- Port the SFML widget onto `adhd_core` so both front-ends share the adaptive engine.
- Smarter adaptation, e.g. use your best hour of the day or per-task focus lengths.
- A proper playlist generator behind the hooks, with music length matched to the phase.
- Body doubling: share a live session with a friend.

## License

MIT, see [LICENSE](LICENSE). Copyright (c) 2025 lm
