## phase2a-inputrouter-fsm-activation (b516f8d)

- src/engine/InputRouter.cpp: accept commit_char JSON and consume the next touch_up to avoid double commits.
- src/engine/InputRouter.h: add m_skipCommitOnTouchUp state flag.
- src/engine/UInputKeyboard.cpp: fix letter keycode mapping (non-contiguous A–Z), register correct keybits, add commit logging, and guard against stuck modifiers.
- src/ui/main.cpp: add sendChar() to send explicit characters to the engine.
- src/ui/qml/RadialKeyboard.qml: commit on release/hover exit using UI glyphs, plus debug log and special handling for space/backspace/enter.
- src/ui/qml/MainOverlay.qml: ergonomic placement and opacity control UI.
