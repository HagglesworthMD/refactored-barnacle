#include <QtTest/QtTest>

#include "../src/engine/RadialLayout.h"
#include "../src/engine/GestureRecognizer.h"
#include "../src/engine/StateMachine.h"

using namespace radialkb;

class EngineTests : public QObject {
    Q_OBJECT

private slots:
    void angleToSectorMaps();
    void swipeClassification();
    void stateTransitions();
};

void EngineTests::angleToSectorMaps() {
    RadialLayout layout({8, 0.5, 0.5, 0.0});
    QCOMPARE(layout.angleToSector(1.0, 0.5), 0);
    QCOMPARE(layout.angleToSector(0.5, 0.0), 6);
    QCOMPARE(layout.angleToSector(0.0, 0.5), 4);
}

void EngineTests::swipeClassification() {
    GestureRecognizer recognizer;
    TouchSample start{0.5, 0.5, 1000};
    TouchSample end{0.8, 0.5, 1100};
    recognizer.onTouchDown(start);
    QCOMPARE(recognizer.onTouchUp(end), SwipeDir::Right);
}

void EngineTests::stateTransitions() {
    StateMachine machine;
    QCOMPARE(machine.state(), State::Idle);
    machine.transitionTo(State::Touching, "test_down");
    QCOMPARE(machine.state(), State::Touching);
    machine.transitionTo(State::Committing, "test_commit");
    QCOMPARE(machine.state(), State::Committing);
}

QTEST_MAIN(EngineTests)
#include "engine_tests.moc"
