# Swipe decoding + haptics (radial trackpad keyboard)

Project constraints (compressed):
- Radial/circular layout only; one-thumb usable; trackpad-first
- Desktop mode overlay must NOT steal focus
- Offline-only; Linux/SteamOS native; designed for text + coding
- Daily usability: stability + minimal diffs over new risk

## Decoder options

### A) SHARK2 / ShapeWriter-style template decoding (recommended MVP)
Core idea:
- Treat a swipe as a noisy observation of an "ideal word path" over the keyboard layout.
- For each candidate word, build its ideal polyline from letter anchor points.
- Score observed swipe vs ideal paths (shape distance / DTW-like) and combine with a language-model prior.
Why it fits us:
- Deterministic, fast, and offline
- Layout-agnostic => works on radial anchors (not tied to QWERTY grids)
- Easy to feature-flag and iterate
Implementation notes (MVP):
- Precompute ideal paths for the active dictionary at startup (or lazily) to keep decoding fast.
- Normalize both observed and ideal paths into a common radius/angle space before scoring.
- Combine a shape-distance score with a unigram prior from an offline word list; allow a user dictionary override.

### B) Neural decoders (optional later)
Core idea:
- Train / pretrain a model to map trajectories to text (often better under noise).
Pros:
- Higher accuracy on messy trajectories (reported in recent XR WGK research)
Cons:
- Model packaging, on-device inference, quantization, evaluation burden
- More moving parts (riskier for daily-usability stage)

## Smooth swipe: what matters most
Even with a good decoder, "smooth" feel is dominated by input conditioning and feedback.

### Path conditioning (engine-side)
- Low-pass filter pointer samples (tiny EMA) to reduce jitter
- Resample to a fixed number of points for decoding (e.g., 64)
- Enforce a minimal movement threshold before adding points
- Time-normalize (or DTW) in the decoder so speed variations don't break matches
Optional extras:
- Snap the first/last sample toward the nearest anchor if the swipe begins/ends very close to it.
- Record per-swipe timing stats so decoder tuning is data-driven without adding telemetry.

### Haptics strategy (trackpad-safe)
Goal: informative, not buzzy.

Trigger haptics only on meaningful events:
1) Sector boundary crossings:
   - When current sector index changes during swipe, emit a short tick
   - Gate by minimum interval (e.g., >= 35ms) to prevent buzzing
2) Candidate stability events:
   - When top-1 candidate remains unchanged for X ms (e.g., 120ms), emit a gentle confirmation tick
3) Commit:
   - On lift/click accept, emit a distinct confirmation pulse

Avoid:
- Haptic on every motion sample
- Long pulses
- High-frequency buzzing during slow movement

## Proposed implementation order (daily usability)
1) Add SHARK2-style decoder module behind a feature flag (no behavior changes by default)
2) Add path conditioning (EMA + resample) in the swipe pipeline
3) Add boundary haptic gating (sector-change ticks) + candidate-stability haptics
4) Later: optional neural decoder behind another flag, keep template decoder fallback
