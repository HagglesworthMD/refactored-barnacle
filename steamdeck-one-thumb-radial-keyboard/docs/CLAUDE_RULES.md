# Claude Rules — Non-Negotiable Constraints

This document defines **what MUST NOT be changed** by AI contributors
(Claude Opus, Codex, etc.) when working on this project.

Violating any rule below is considered a hard regression.

---

## 1. Input & Layout Rules (Absolute)

- ❌ No QWERTY grids (visible or hidden)
- ❌ No rectangular key matrices
- ❌ No touchscreen-first assumptions
- ❌ No two-handed input assumptions
- ❌ No mouse-only designs

✔ Radial / circular layout only  
✔ One-thumb usable  
✔ Trackpad-first (Steam Deck + Apple Magic Trackpad)

---

## 2. Focus & Windowing Rules (Critical)

- ❌ Overlay must NEVER steal keyboard focus
- ❌ Overlay must NEVER break the active application’s input context
- ❌ No modal dialogs
- ❌ No focus grabs, `activate()`, or input method hijacking

✔ Overlay is visual + gesture-driven only  
✔ Always-on-top is allowed  
✔ Non-focus-stealing flags must remain intact

---

## 3. Platform & Environment Rules

- ❌ No cloud APIs
- ❌ No online dependencies
- ❌ No telemetry
- ❌ No accounts / login flows

✔ Offline-only  
✔ Linux / SteamOS native  
✔ Desktop mode compatible  

---

## 4. Engine Authority Rules

- ❌ UI must NOT interpret gestures or decide commits
- ❌ QML must NOT become the source of truth
- ❌ Layout logic must NOT be hard-coded in the engine

✔ `InputRouter` FSM is authoritative  
✔ Layout defines keys; engine commits via `KeyOption`  
✔ At-most-once commit per gesture session  

---

## 5. Ergonomics Rules

- ❌ No precision-heavy mechanics
- ❌ No rapid vibration spam
- ❌ No cognitive overload features

✔ Favor forgiveness over accuracy  
✔ Reduce thumb effort  
✔ Cancel must always be easy and reliable  

---

## 6. Change Philosophy

- Prefer **minimal diffs**
- Avoid refactors unless explicitly requested
- Stability > cleverness
- Logs should be high-signal and human-readable

If unsure: **do less, not more**.

