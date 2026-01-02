#include "InputRouter.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

#include "Logging.h"

namespace radialkb {

InputRouter::InputRouter(QObject *parent)
    : QObject(parent) {
}

QString InputRouter::handleMessage(const QString &line) {
    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        Logging::log(LogLevel::Warn, "ENGINE", QString("invalid json: %1").arg(error.errorString()));
        return "{\"error\":\"invalid_json\"}";
    }

    const QJsonObject obj = doc.object();
    const QString type = obj.value("type").toString();
    if (type == "touch_down") {
        handleTouchDown(obj.value("x").toDouble(), obj.value("y").toDouble());
    } else if (type == "touch_move") {
        handleTouchMove(obj.value("x").toDouble(), obj.value("y").toDouble());
    } else if (type == "touch_up") {
        handleTouchUp(obj.value("x").toDouble(), obj.value("y").toDouble());
    } else if (type == "action") {
        handleAction(obj.value("action").toString());
    } else if (type == "ui_show") {
        m_stateMachine.transitionTo(State::Idle, "ui_show");
    } else if (type == "ui_hide") {
        m_stateMachine.transitionTo(State::Hidden, "ui_hide");
    }

    QJsonObject reply;
    reply.insert("ack", true);
    if (type == "touch_move" && m_selectedSector >= 0) {
        reply.insert("type", "selection");
        reply.insert("sector", m_selectedSector);
    } else {
        reply.insert("type", "ack");
    }

    return QJsonDocument(reply).toJson(QJsonDocument::Compact);
}

void InputRouter::handleTouchDown(double xNorm, double yNorm) {
    m_lastX = xNorm;
    m_lastY = yNorm;
    TouchSample sample{xNorm, yNorm, QDateTime::currentMSecsSinceEpoch()};
    m_gestures.onTouchDown(sample);
    m_stateMachine.transitionTo(State::Touching, "touch_down");
}

void InputRouter::handleTouchMove(double xNorm, double yNorm) {
    m_lastX = xNorm;
    m_lastY = yNorm;
    TouchSample sample{xNorm, yNorm, QDateTime::currentMSecsSinceEpoch()};
    const SwipeDir swipe = m_gestures.onTouchMove(sample);
    if (swipe != SwipeDir::None) {
        m_stateMachine.transitionTo(State::GestureCandidate, "fast_move");
        Logging::log(LogLevel::Debug, "GESTURE", QString("candidate %1").arg(swipeToString(swipe)));
        return;
    }
    m_stateMachine.transitionTo(State::Sliding, "touch_move");
    const int sector = m_layout.angleToSector(xNorm, yNorm);
    if (sector != m_selectedSector) {
        m_selectedSector = sector;
        m_haptics.onSelectionChange();
        emit selectionChanged(sector);
        Logging::log(LogLevel::Info, "ENGINE", QString("selection sector %1").arg(sector));
    }
}

void InputRouter::handleTouchUp(double xNorm, double yNorm) {
    TouchSample sample{xNorm, yNorm, QDateTime::currentMSecsSinceEpoch()};
    const SwipeDir swipe = m_gestures.onTouchUp(sample);
    if (swipe == SwipeDir::Left) {
        m_commit.commitAction("backspace");
        m_stateMachine.transitionTo(State::Committing, "swipe_left");
    } else if (swipe == SwipeDir::Right) {
        m_commit.commitAction("space");
        m_stateMachine.transitionTo(State::Committing, "swipe_right");
    } else if (swipe == SwipeDir::Down) {
        m_stateMachine.transitionTo(State::Cancelled, "swipe_down");
    } else {
        if (m_selectedSector < 0) {
            m_selectedSector = 0;
        }
        QChar ch = m_layout.sectorList().at(m_selectedSector).primaryChar;
        if (ch.isNull()) {
            Logging::log(LogLevel::Warn, "ENGINE", "null char in layout, committing '?'");
            ch = QChar('?');
        }
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
