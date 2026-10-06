# Changelog

## 2.0.0 - 2026-10-06

A rebuild around the original idea, as a cross-platform terminal app with no dependencies.

### Added
- `adhd_core` library: a testable Pomodoro engine (injectable clock, event-driven) with the
  Focus → Extended Focus → Hyperfocus → Rest → Idle cycle.
- **Adaptive focus length**: good/bad ratings now actually tune the next session (±2 min, bounded).
- **Proportional rest**: long sessions earn longer breaks (20% of focus time, capped). The v1 GUI computed this but never used it.
- **Parking lot** (`d`) to capture distracting thoughts into `parked.md` without leaving focus.
- **Tiny start** (`m`): a 5-minute session for when starting feels impossible.
- Nudges in Extended Focus / Idle and body check-ins during Hyperfocus; terminal bell, optional desktop notifications.
- Phase hooks (`hook.focus = ...`) to drive music players or other apps.
- `sessions.csv` journal and an `adhd-pomodoro stats` report (streak, 7-day sparkline, ratings, best hour, restart time).
- Full-screen terminal UI: big digits, progress bar, per-phase colours, settings screen, help/stats overlays, live tab title.
- Unit tests (`ctest`) and CI on Linux, macOS and Windows.

### Changed
- The CLI binary is now `adhd-pomodoro`. Data lives in a per-user folder (`adhd-pomodoro where`).
- Settings use a commented `key = value` file. A v1 `settings.txt` in the working directory is imported on first run.
- The SFML widget moved to `src/gui/`.

### Removed
- The v1 CLI (`cli_main.cpp`, `CLISettings`), replaced by the above.

## 2025-05-15 - SFML GUI

### Added
- Settings window functionality with separate window
- Start button to initiate timer instead of auto-start
- Pause and Finish buttons with logging
- Visual feedback for settings menu hover
- Debug output for click detection

### Fixed
- Settings button click detection
- Text visibility in settings window
- Window management and event handling
- Access control issues in Settings class

### Improved
- UI elements with better colors and contrast
- Settings window layout and usability
- Click detection with larger hit areas
- Logging system for timer events

### Technical Details
- Added public methods for window management in Settings class
- Implemented proper event handling for both main and settings windows
- Enhanced text rendering with better visibility
- Added debug logging for troubleshooting 
