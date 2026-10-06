# Quick start

```bash
cmake -S . -B build -DBUILD_CLI_ONLY=ON
cmake --build build --config Release
./build/adhd-pomodoro
```

1. Press `s` to start a focus session (or `m` for a 5-minute tiny start if starting feels hard).
2. Distracting thought? `d`, type it, Enter. It waits in `parked.md` for your break.
3. When the timer runs out you're **not** interrupted. Keep going as long as it flows.
4. When you surface, rate it: `g` good, `o` okay, `b` bad. Ratings tune your next focus length.
5. Rest. When the break ends, press `s` again.

Press `?` in the app for all keys, `i` for stats, `c` for settings.
Run `./build/adhd-pomodoro stats` any time to see how you're doing.
