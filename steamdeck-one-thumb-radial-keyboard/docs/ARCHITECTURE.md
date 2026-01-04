# Architecture Overview (RadialKB / TouchKB)

This document describes the current architecture and intended extension points for:
- FSM-based input routing
- Input pipeline (touchscreen / trackpad → interpretation → action)
- Haptics hooks
- UI overlay behavior (non-focus-stealing)

It is written to help contributors (including AI agents) make changes safely.

---

## Input Modes

The project supports two input modes:

### Touch Mode (iOS-style keyboard)
- Enabled via `RADIALKB_INPUT=touch`
- Uses libinput for touchscreen input (Steam Deck FTS3528)
- iOS-style rectangular QWERTY layout
- Physical sizing based on millimeters (48mm keyboard height)
- Tap-to-commit interaction

### Radial Mode (default)
- Enabled when `RADIALKB_INPUT` is unset or not "touch"
- Uses trackpad input via UI socket
- Radial/circular layout with 8 sectors
- One-thumb usable on Steam Deck left trackpad

---

## Non-negotiable Constraints

- One-thumb usable
- Desktop mode compatible
- Overlay must NOT steal focus
- Offline-only
- Linux / SteamOS native
- Avoid regressions; stability-first

---

## High-level Components

### Engine (radialkb-engine)
Responsibilities:
- Capture/receive normalized pointer events (trackpad/mouse-like input)
- Route events through an InputRouter FSM
- Emit commit actions (characters, backspace, enter, etc.)
- Drive optional haptics feedback (sector boundaries, commit confirmation)
- Communicate with UI (state updates, candidates, overlay commands) via IPC/DBus as implemented

Key files (expected):
- `src/engine/InputRouter.{h,cpp}` — routing + FSM
- `src/engine/*` — action emission, IPC, device injection

### UI (radialkb-ui)
Responsibilities:
- Render overlay (radial sectors or touch keyboard, highlights, candidate row)
- Never steal focus from the target app
- Provide visual feedback for current sector/selection
- Offer controls (pin/show/hide, drag handle, mode toggles) where applicable

Key files:
- `src/ui/main.cpp` — UI entry point, UiBridge, ScreenMetrics
- `src/ui/qml/MainOverlay.qml` — Root window, mode switching
- `src/ui/qml/RadialKeyboard.qml` — Radial trackpad keyboard
- `src/ui/qml/TouchKeyboard.qml` — iOS-style touchscreen keyboard

---

## Touchscreen Input (libinput)

When `RADIALKB_INPUT=touch`, the engine uses libinput to capture touchscreen events.

### TouchInputLibinput
- Uses libinput PATH backend (not udev) for direct device access
- Integrates with Qt event loop via QSocketNotifier
- Normalizes touch coordinates from mm to 0..1 space
- Device path configurable via `RADIALKB_TOUCH_DEVICE` (default: `/dev/input/event15`)

Physical dimensions (Steam Deck FTS3528):
- Width: 267mm
- Height: 142mm

Signals:
- `touchDown(int slot, double xNorm, double yNorm)`
- `touchMove(int slot, double xNorm, double yNorm)`
- `touchUp(int slot)`

### ScreenMetrics
Provides physical sizing for QML:
- `pxPerMm` — pixels per millimeter (~4.79 for Steam Deck)
- `mmToPx(mm)` — convert mm to pixels
- `keyboardHeightMm` — iOS-like keyboard height (48mm)

---

## Input Pipeline

### 1) Raw input → Normalization
Input arrives as positions/deltas. Normalize to a stable coordinate space:
- Prefer 0..1 normalized coordinates for internal state
- Track touch down/up vs hover/move vs click/press if available
- Maintain a robust “gesture session” concept from touch-down → touch-up/cancel

### 2) Interpretation (FSM)
The FSM decides whether the user is:
- Selecting a sector
- Committing a character
- Performing a swipe gesture (space/backspace/cancel)
- Cancelling due to leaving pad bounds or explicit cancel gesture

This must be resilient to:
- Jitter near boundaries
- Touch-up races
- Short accidental swipes

### 3) Action emission
Once an action is committed:
- Emit the action once (idempotent, no double-commit)
- Reset the FSM to a neutral state
- UI receives the committed glyph (optional) for visual confirmation

### 4) Haptics feedback
Haptics should be driven from *meaningful transitions*:
- Sector boundary crossed (tick)
- Commit occurred (pulse)
- Cancel (distinct cue)

Never vibrate continuously; never spam.

---

## FSM Design (Recommended Shape)

States (example):
- `Idle`
- `TouchActive` (tracking contact, waiting for intent)
- `SelectingSector` (active selection, boundary haptics)
- `SwipeGesture` (direction classification + thresholds)
- `CommitPending` (commit on lift/click; consumes next touch-up safely)
- `Cancelled`

Key transitions:
- Idle -> TouchActive on touch-down
- TouchActive -> SelectingSector as movement indicates selection
- SelectingSector -> CommitPending when a commit condition is met (lift/click)
- TouchActive/SelectingSector -> SwipeGesture when movement exceeds swipe threshold
- Any -> Cancelled on swipe-down/cancel command/pad exit
- CommitPending -> Idle after commit and touch-up consumed

Core rules:
- A commit must happen at most once per gesture session
- Cancel must always win over commit when clearly requested
- Touch-up handling must be carefully ordered to avoid “commit then swipe” races

---

## Haptics Hooks

Haptics should be abstracted behind a small interface:
- `haptics.tickBoundary()` when sector index changes
- `haptics.pulseCommit()` on commit
- `haptics.pulseCancel()` on cancel

Suggested placement:
- Inside the InputRouter FSM on state transitions, not in UI rendering code
- UI may optionally mirror visual feedback but should not be the authority

---

## UI Overlay: Non-focus-stealing Rules

The overlay must:
- Not take keyboard focus
- Not disrupt the active application input context
- Render above content (always-on-top) without intercepting normal typing focus

Auto show/hide:
- Show when a text field is focused (where possible)
- Hide when focus leaves (with hysteresis to avoid flicker)
- Provide a “pin” toggle to keep visible

Drag behavior:
- Draggable handle that does not steal focus
- Position persistence (store last x/y)
- Snap zones optional (future)

---

## Safe Extension Points (for future phases)

Phase 2:
- Swipe typing: capture continuous path, score dictionary (offline), return top candidates
- UI candidates row; accept by click/lift; reject by cancel swipe

Phase 3:
- Gesture glyphs: unistroke recognizer
- Map glyphs to editing + coding actions

Phase 4:
- Prediction/learning: local-only vocabulary adaptation; code/text mode

---

## Testing & Regression Strategy

- Deterministic logs for FSM transitions (high-signal tags)
- Avoid changing masking/anonymisation logic (not applicable here), focus on stability
- Manual smoke test:
  - sector select → commit
  - swipe left/right/down
  - cancel paths reset state
  - overlay doesn’t steal focus in a target app (e.g. browser address bar)
