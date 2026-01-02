# State Machine

## States
- Hidden
- Idle
- Touching
- Sliding
- GestureCandidate
- Committing
- Cancelled

## Transitions
- Hidden -> Idle (ui_show)
- Idle -> Touching (touch_down)
- Touching -> Sliding (touch_move)
- Sliding -> GestureCandidate (fast_move)
- GestureCandidate -> Committing (swipe_left/right)
- GestureCandidate -> Cancelled (swipe_down)
- Sliding -> Committing (touch_up_commit)
- Cancelled -> Idle (cancel_done)
- Committing -> Idle (commit_done)

All transitions are logged with reasons via `[FSM]`.
