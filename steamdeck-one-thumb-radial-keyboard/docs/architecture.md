# Architecture

## Modules
- **UI (Qt/QML)**: renders overlay, captures trackpad-like input, sends IPC messages.
- **Engine (Qt Core)**: input router, state machine, gesture recognition, layout mapping, commit bridge.
- **Commit Bridge (stub)**: logs commit actions; future Fcitx5 addon integration.

## Message Flow (UI <-> Engine)
```
UI (QLocalSocket)
  -> {"type":"touch_down","x":0.1,"y":-0.2}
  -> {"type":"touch_move","x":0.2,"y":-0.1}
  -> {"type":"touch_up","x":0.2,"y":-0.1}
  -> {"type":"action","action":"backspace"}

Engine
  - InputRouter parses JSON
  - StateMachine transitions logged
  - GestureRecognizer classifies swipe
  - CommitBridge logs commit
  <- {"ack":true}
```

## Logging Tags
- `[UI]` UI side events
- `[ENGINE]` Engine actions
- `[FSM]` State transitions
- `[GESTURE]` Gesture classification
- `[COMMIT]` Commit bridge actions
