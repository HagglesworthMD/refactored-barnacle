# Input Model

## Gesture Thresholds (Engine)
Defined in `src/engine/GestureRecognizer.h`:
- `minDistanceNorm`: 0.12 (normalized [0..1])
- `minVelocityNormPerMs`: 0.0009
- `maxDurationMs`: 220ms

## Rationale
- Short, fast swipes trigger actions (backspace/space/cancel).
- Slower motion stays in Sliding state for sector selection.

## Default Actions
- Tap / Lift -> commit selected letter
- Swipe Left -> Backspace
- Swipe Right -> Space
- Swipe Down -> Cancel
- Hard Click (mapped to right-click in UI) -> Enter
