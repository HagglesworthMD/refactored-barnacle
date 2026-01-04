# PR: Phase 2a InputRouter FSM activation + commit-path fixes

## Summary
This PR improves the one-thumb radial keyboard commit path and input reliability:

- Prevents double-commits by consuming the next `touch_up` after a pending commit.
- Accepts explicit `commit_char` from UI and logs actual committed characters.
- Fixes uinput letter mapping (non-contiguous A–Z), registers correct keybits,
  guards against stuck modifiers, and adds commit logging.
- UI: commits on release/hover-exit using UI glyphs; special handling for space/backspace/enter.
- Overlay: ergonomic placement and opacity control UI.

## Files changed
- `src/engine/InputRouter.cpp`
- `src/engine/InputRouter.h`
- `src/engine/UInputKeyboard.cpp`
- `src/ui/main.cpp`
- `src/ui/qml/RadialKeyboard.qml`
- `src/ui/qml/MainOverlay.qml`

## Testing notes (manual)
- Built + installed to `~/.local` and restarted:
  - `radialkb-engine.service`
  - `radialkb-ui.service`
- Verified:
  - No double-commit when lifting after selection
  - Space/backspace/enter behave as expected
  - Modifiers do not stick (guarded)
  - Logs show committed glyph + actual sent character
