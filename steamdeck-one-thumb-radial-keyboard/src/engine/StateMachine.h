#pragma once

#include <QString>

namespace radialkb {

enum class State {
    Hidden,
    Idle,
    Touching,
    Sliding,
    GestureCandidate,
    Committing,
    Cancelled
};

struct Transition {
    State from;
    State to;
    QString reason;
};

class StateMachine {
public:
    StateMachine();

    State state() const;
    Transition transitionTo(State next, const QString &reason);

private:
    State m_state;
};

QString stateToString(State state);

}
