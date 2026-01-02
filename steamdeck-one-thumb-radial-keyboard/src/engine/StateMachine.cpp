#include "StateMachine.h"

#include "Logging.h"

namespace radialkb {

StateMachine::StateMachine()
    : m_state(State::Idle) {
}

State StateMachine::state() const {
    return m_state;
}

Transition StateMachine::transitionTo(State next, const QString &reason) {
    Transition t{m_state, next, reason};
    m_state = next;
    logWithTag("[FSM]", QString("%1 -> %2 (%3)")
        .arg(stateToString(t.from))
        .arg(stateToString(t.to))
        .arg(reason));
    return t;
}

QString stateToString(State state) {
    switch (state) {
    case State::Hidden:
        return "Hidden";
    case State::Idle:
        return "Idle";
    case State::Touching:
        return "Touching";
    case State::Sliding:
        return "Sliding";
    case State::GestureCandidate:
        return "GestureCandidate";
    case State::Committing:
        return "Committing";
    case State::Cancelled:
        return "Cancelled";
    }
    return "Unknown";
}

}
