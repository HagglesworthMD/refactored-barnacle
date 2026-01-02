# Steam Deck One-Thumb Radial Keyboard

A one-thumb, trackpad-first, radial on-screen keyboard overlay for Steam Deck trackpads and Apple Magic Trackpad (docked). Designed for coding and text, with a non-focus-stealing overlay and an engine that commits text into the currently focused app.

## Phase 1 MVP Goals
- Radial overlay (8 sectors, configurable up to 12)
- Trackpad-first input model (tap, slide, lift, swipe)
- Explicit state machine with logging
- Local IPC between UI and engine
- Stubbed commit bridge + haptics hooks

## Build
```bash
cmake -S . -B build
cmake --build build
```

## Run (Dev)
```bash
./packaging/scripts/run-dev.sh
```

## Systemd User Services
See `packaging/systemd/` and `packaging/scripts/install-user.sh`.

## Notes
- Overlay does **not** steal focus.
- Commit bridge is a stub in MVP, logs to stdout.
