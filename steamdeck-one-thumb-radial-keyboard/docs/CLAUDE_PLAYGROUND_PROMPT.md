You are Claude Opus 4.5 working on the project:

**Steam Deck One-Thumb Radial Trackpad Keyboard**

This is an experimental playground branch. You are encouraged to explore ideas,
but you MUST respect the project constraints and rules.

Before making any changes:
1. Read:
   - docs/FEATURES.md
   - docs/ARCHITECTURE.md
   - docs/CLAUDE_RULES.md

---

## Your Mission

Improve typing speed, confidence, and comfort for **one-thumb trackpad input**
without violating any non-negotiable constraints.

You may explore:
- Haptics tuning (boundary ticks, commit pulses, cancel cues)
- Magnetic / assist zones that reduce precision demand
- Swipe typing (radial path based, not QWERTY)
- Gesture glyphs for editing & coding
- Draggable UI (non-focus-stealing)
- Auto show/hide with hysteresis
- Prediction & learning (local-only)

---

## Hard Constraints (Do Not Violate)

- Radial / circular layout only
- One-thumb usable
- Trackpad-first
- Overlay must NOT steal focus
- Offline-only
- Linux / SteamOS native
- No QWERTY grids
- No touchscreen assumptions

---

## Where You Are Allowed to Work

Safe areas:
- docs/
- New experimental files
- Small, well-contained changes in:
  - InputRouter FSM (with extreme care)
  - Haptics hooks
  - UI positioning / rendering (no focus changes)

Avoid:
- Large refactors
- Changing input ownership
- Rewriting gesture semantics unless explicitly justified

---

## How to Propose Changes

For each idea:
1. Explain the ergonomic benefit
2. Explain why it respects constraints
3. Show the minimal diff
4. Describe failure modes and how cancel/reset behaves

Your goal is not novelty — it is **effort reduction**.

If a feature makes typing feel heavier, slower, or less predictable, it is wrong.

