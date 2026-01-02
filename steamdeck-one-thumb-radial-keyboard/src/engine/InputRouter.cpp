#include "InputRouter.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtMath>
#include <algorithm>

#include "Logging.h"

namespace radialkb {

InputRouter::InputRouter(QObject *parent)
    : QObject(parent),
      m_layout(RadialLayoutConfig{8, 0.5, 0.5, M_PI / 2.0}) {
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
    if ((type == "touch_move" || type == "touch_down") && m_selectedSector >= 0) {
        reply.insert("type", "selection");
        reply.insert("sector", m_selectedSector);
        reply.insert("letter", m_selectedKey);
        reply.insert("stage", m_trackingLetter ? "letter" : "group");
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
    enterTrackGroup("touch_down");
    updateSelection(xNorm, yNorm);
}

void InputRouter::handleTouchMove(double xNorm, double yNorm) {
    m_lastX = xNorm;
    m_lastY = yNorm;
    TouchSample sample{xNorm, yNorm, QDateTime::currentMSecsSinceEpoch()};
    const SwipeDir swipe = m_gestures.onTouchMove(sample);
    if (swipe == SwipeDir::Down) {
        m_stateMachine.transitionTo(State::Cancelled, "swipe_down");
        m_haptics.onCancel();
        m_stateMachine.transitionTo(State::Idle, "cancel_done");
        return;
    }
    updateSelection(xNorm, yNorm);
}

void InputRouter::handleTouchUp(double xNorm, double yNorm) {
    TouchSample sample{xNorm, yNorm, QDateTime::currentMSecsSinceEpoch()};
    const SwipeDir swipe = m_gestures.onTouchUp(sample);
    if (swipe == SwipeDir::Left) {
        m_commit.commitAction("backspace");
        m_stateMachine.transitionTo(State::Committing, "swipe_left");
        m_haptics.onCommit();
        m_stateMachine.transitionTo(State::Idle, "commit_done");
        return;
    }
    else if (swipe == SwipeDir::Right) {
        m_commit.commitAction("space");
        m_stateMachine.transitionTo(State::Committing, "swipe_right");
        m_haptics.onCommit();
        m_stateMachine.transitionTo(State::Idle, "commit_done");
        return;
    }
    else if (swipe == SwipeDir::Down) {
        m_stateMachine.transitionTo(State::Cancelled, "swipe_down");
        m_haptics.onCancel();
        m_stateMachine.transitionTo(State::Idle, "cancel_done");
        return;
    }

    updateSelection(xNorm, yNorm);
    if (m_selectedSector < 0) {
        m_stateMachine.transitionTo(State::Idle, "touch_up_no_selection");
        return;
    }

    const int keyCount = m_layout.keyCount(m_selectedSector);
    if (keyCount <= 0) {
        Logging::log(LogLevel::Warn, "ENGINE", "empty sector keys, skipping commit");
        m_stateMachine.transitionTo(State::Idle, "commit_done");
        return;
    }
    const int keyIndex = (m_trackingLetter && m_selectedKey >= 0)
        ? m_selectedKey
        : 0;
    const int clampedIndex = std::clamp(keyIndex, 0, keyCount - 1);
    const KeyOption &key = m_layout.keyAt(m_selectedSector, clampedIndex);
    if (key.isAction()) {
        m_commit.commitAction(key.action);
    } else if (!key.ch.isNull()) {
        m_commit.commitChar(key.ch);
    } else {
        Logging::log(LogLevel::Warn, "ENGINE", "no key payload to commit");
    }
    m_stateMachine.transitionTo(State::Committing, "touch_up_commit");
    m_haptics.onCommit();
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
        m_haptics.onCancel();
        m_stateMachine.transitionTo(State::Idle, "cancel_done");
    }
}

void InputRouter::updateSelection(double xNorm, double yNorm) {
    constexpr double kDeadzoneRadius = 0.12;
    constexpr double kInnerRadius = 0.28;
    constexpr double kInnerHysteresis = 0.03;
    constexpr double kAngleHysteresis = (3.0 * M_PI / 180.0);

    const double angle = m_layout.angleForPoint(xNorm, yNorm);
    const double radius = m_layout.radiusForPoint(xNorm, yNorm);

    if (!m_trackingLetter && radius >= kInnerRadius) {
        enterTrackLetter("enter_inner");
    } else if (m_trackingLetter && radius < (kInnerRadius - kInnerHysteresis)) {
        enterTrackGroup("exit_inner");
    }

    if (radius < kDeadzoneRadius) {
        if (m_selectedSector != -1 || m_selectedKey != -1) {
            m_selectedSector = -1;
            m_selectedKey = -1;
            emit selectionChanged(m_selectedSector, m_selectedKey, "group");
            Logging::log(LogLevel::Info, "ENGINE", "selection cleared (deadzone)");
        }
        return;
    }

    const int nextSector = m_layout.angleToSectorWithHysteresis(angle, m_selectedSector, kAngleHysteresis);
    if (nextSector != m_selectedSector) {
        m_selectedSector = nextSector;
        m_selectedKey = -1;
        m_haptics.onSelectionChange();
        emit selectionChanged(m_selectedSector, m_selectedKey, m_trackingLetter ? "letter" : "group");
        Logging::log(LogLevel::Info, "ENGINE", QString("selection sector %1").arg(m_selectedSector));
    }

    if (m_trackingLetter && m_selectedSector >= 0) {
        const int nextKey = m_layout.angleToKeyIndexWithHysteresis(
            angle, m_selectedSector, m_selectedKey, kAngleHysteresis);
        if (nextKey != m_selectedKey) {
            m_selectedKey = nextKey;
            m_haptics.onSelectionChange();
            emit selectionChanged(m_selectedSector, m_selectedKey, "letter");
            Logging::log(LogLevel::Info, "ENGINE",
                         QString("selection key %1:%2").arg(m_selectedSector).arg(m_selectedKey));
        }
    } else if (!m_trackingLetter) {
        m_selectedKey = -1;
    }
}

void InputRouter::enterTrackGroup(const QString &reason) {
    m_trackingLetter = false;
    m_stateMachine.transitionTo(State::TrackGroup, reason);
}

void InputRouter::enterTrackLetter(const QString &reason) {
    m_trackingLetter = true;
    m_stateMachine.transitionTo(State::TrackLetter, reason);
}

}
