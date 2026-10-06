# ADHD Pomodoro: contributor notes

An ADHD-friendly Pomodoro timer in C++17. The main app is a dependency-free terminal program
that builds on Linux, macOS and Windows. A legacy SFML always-on-top widget lives in `src/gui`.

## Layout

- `src/core/`: platform-free logic (the `adhd_core` static library). No terminal, threads or sleeping.
  - `Engine`: phase state machine. Time comes from a `Clock` (use `ManualClock` in tests). Results are
    reported via `Engine::Event` (PhaseChanged / PhaseCompleted / Nudge / FocusAdjusted).
  - `Config` (settings file, incl. legacy v1 `"<focus> <rest>"`), `Journal` (CSV log), `Stats`,
    `ParkingLot`, `Paths`, `TimeUtil`, `Phase`.
- `src/cli/`: terminal front-end. `Terminal` (raw input, POSIX + Windows), `Render` (ANSI helpers, big
  digits), `App` (full-screen UI, wires engine events to journal/bell/hooks), `StatsView`, `main.cpp` (args).
- `src/gui/`: v1 SFML widget, Windows only. Not built with `-DBUILD_CLI_ONLY=ON`.
- `tests/test_main.cpp`: self-contained test runner with `TEST` / `CHECK` / `CHECK_EQ` macros.

## Build and test

```bash
cmake -S . -B build -DBUILD_CLI_ONLY=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Builds take seconds. The build should stay warning-free under `-Wall -Wextra -Wpedantic -Wshadow`.

## Conventions

- Keep `src/core` free of I/O other than plain files, so it stays unit-testable. Add a test for any engine
  or stats change. Use `ManualClock` to fast-forward time instead of sleeping.
- Platform-specific code belongs in `src/cli/Terminal.cpp` behind `#ifdef _WIN32`.
- The interactive timer needs a TTY. `stats`, `parked` and `where` work anywhere and are useful for smoke tests.
  Pass `--data-dir <tmp>` so test runs don't touch real user data.
