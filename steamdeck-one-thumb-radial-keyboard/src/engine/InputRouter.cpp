#include "InputRouter.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QtMath>

#include "Logging.h"

namespace radialkb {

InputRouter::InputRouter(QObject *parent)
    : QObject(parent) {
}

QString InputRouter::handleMessage(const QString &line) {
    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        logWithTag("[ENGINE]", QString("invalid json: %1").arg(error.errorString()));
        return "{\"error\":\"invalid_json\"}";
    }

    const QJsonObject obj = doc.object();
    const QString type = obj.value("type").toString();
    if (type == "touch_down") {
        const QPointF pos(obj.value("x").toDouble(), obj.value("y").toDouble());
        handleTouchDown(pos);
    } else if (type == "touch_move") {
        const QPointF pos(obj.value("x").toDouble(), obj.value("y").toDouble());
        handleTouchMove(pos);
    } else if (type == "touch_up") {
        const QPointF pos(obj.value("x").toDouble(), obj.value("y").toDouble());
        handleTouchUp(pos);
    } else if (type == "action") {
        handleAction(obj.value("action").toString());
    } else if (type == "ui_show") {
        m_stateMachine.transitionTo(State::Idle, "ui_show");
    } else if (type == "ui_hide") {
        m_stateMachine.transitionTo(State::Hidden, "ui_hide");
    }

    return "{\"ack\":true}";
}

void InputRouter::handleTouchDown(const QPointF &pos) {
    m_lastPos = pos;
    m_gestures.onTouchDown(pos);
    m_stateMachine.transitionTo(State::Touching, "touch_down");
}

void InputRouter::handleTouchMove(const QPointF &pos) {
    m_lastPos = pos;
    const SwipeDirection swipe = m_gestures.onTouchMove(pos);
    if (swipe != SwipeDirection::None) {
        m_stateMachine.transitionTo(State::GestureCandidate, "fast_move");
        logWithTag("[GESTURE]", QString("candidate %1").arg(swipeToString(swipe)));
        return;
    }
    m_stateMachine.transitionTo(State::Sliding, "touch_move");
    const double angle = std::atan2(pos.y(), pos.x());
    const int sector = m_layout.angleToSector(angle);
    if (sector != m_selectedSector) {
        m_selectedSector = sector;
        m_haptics.onSelectionChange();
        emit selectionChanged(sector);
        logWithTag("[ENGINE]", QString("selection sector %1").arg(sector));
    }
}

void InputRouter::handleTouchUp(const QPointF &pos) {
    const SwipeDirection swipe = m_gestures.onTouchUp(pos);
    if (swipe == SwipeDirection::Left) {
        m_commit.commitAction("backspace");
        m_stateMachine.transitionTo(State::Committing, "swipe_left");
    } else if (swipe == SwipeDirection::Right) {
        m_commit.commitAction("space");
        m_stateMachine.transitionTo(State::Committing, "swipe_right");
    } else if (swipe == SwipeDirection::Down) {
        m_stateMachine.transitionTo(State::Cancelled, "swipe_down");
    } else {
        if (m_selectedSector < 0) {
            m_selectedSector = 0;
        }
        const QChar ch = m_layout.sectors().at(m_selectedSector).primaryChar;
        m_commit.commitChar(ch);
        m_stateMachine.transitionTo(State::Committing, "touch_up_commit");
        m_haptics.onCommit();
    }
    m_stateMachine.transitionTo(State::Idle, "commit_done");
}

void InputRouter::handleAction(const QString &actionType) {
    if (actionType == "enter" || actionType == "space" || actionType == "backspace") {
        m_commit.commitAction(actionType);
        m_stateMachine.transitionTo(State::Committing, actionType);
        m_stateMachine.transitionTo(State::Idle, "commit_done");
    }
    if (actionType == "cancel") {
        m_stateMachine.transitionTo(State::Cancelled, "cancel_action");
        m_stateMachine.transitionTo(State::Idle, "cancel_done");
    }
}

}
