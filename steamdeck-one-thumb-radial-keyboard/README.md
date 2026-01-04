# Steam Deck Keyboard

A non-focus-stealing on-screen keyboard for Steam Deck Desktop Mode. Supports two input modes:

- **Touch Mode**: iOS-style touchscreen QWERTY keyboard with physical mm-based sizing
- **Radial Mode**: One-thumb radial keyboard for trackpad input

Designed for daily use, with text committed via uinput into the currently focused app.

## Input Modes

### Touch Mode (iOS-style)
```bash
RADIALKB_INPUT=touch ./radialkb-engine &
./radialkb-ui
```
- Full-width keyboard at screen bottom
- Physical sizing: 48mm height, 8.5mm keys
- Tap-to-commit interaction

### Radial Mode (default)
```bash
./radialkb-engine &
./radialkb-ui
```
- Radial overlay (8 sectors) on left trackpad
- Swipe gestures for space/backspace/cancel

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

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `RADIALKB_INPUT` | (unset) | Set to `touch` for touchscreen mode |
| `RADIALKB_TOUCH_DEVICE` | `/dev/input/event15` | libinput device path for touchscreen |

## Notes
- Overlay does **not** steal focus.
- Commit bridge uses Linux uinput to inject keys into the focused app.
- uinput requires `/dev/uinput` access (try `modprobe uinput`, add your user to the `input` group, then re-login).
- Touchscreen mode requires libinput and access to the touch device (add user to `input` group).

## Manual Test Plan (Desktop Mode)

### Touch Mode
1. Start with `RADIALKB_INPUT=touch`
2. Focus a text field (Kate, Firefox, terminal)
3. Tap keys on the touchscreen keyboard
4. Confirm characters appear in the focused app

### Radial Mode
1. Start without `RADIALKB_INPUT` set
2. Focus a text field
3. Use the radial keyboard to commit letters, spaces, backspace, and enter
4. Confirm input appears in the focused app without the overlay stealing focus
