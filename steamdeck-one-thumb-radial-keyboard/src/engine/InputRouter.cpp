#include "InputRouter.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QChar>
#include <QJsonObject>
#include <QtMath>
#include <algorithm>

#include "Logging.h"

// INTENT: Event ordering here is critical. Prevent commit+swipe races by consuming touch-up
// INTENT: associated with pending commits before gesture classification. Prefer minimal diffs.

namespace radialkb {

namespace {

KeyAction keyOptionToAction(const KeyOption &option) {
    if (option.isAction()) {
        if (option.action == "space") {
            return KeyAction::make(KeyAction::Space);
        }
        if (option.action == "backspace") {
            return KeyAction::make(KeyAction::Backspace);
        }
        if (option.action == "enter") {
            return KeyAction::make(KeyAction::Enter);
        }
        return KeyAction::make(KeyAction::None);
    }
    if (!option.ch.isNull()) {
        return KeyAction::makeChar(option.ch.toLatin1());
    }
    return KeyAction::make(KeyAction::None);
}

QString keycodeLabel(const KeyAction &action) {
    switch (action.type) {
    case KeyAction::Char:
        if (action.ch >= 'a' && action.ch <= 'z') {
            return QString("KEY_%1").arg(QChar(action.ch).toUpper());
        }
        return QStringLiteral("KEY_UNKNOWN");
    case KeyAction::Space:
        return QStringLiteral("KEY_SPACE");
    case KeyAction::Backspace:
        return QStringLiteral("KEY_BACKSPACE");
    case KeyAction::Enter:
        return QStringLiteral("KEY_ENTER");
    case KeyAction::None:
        return QStringLiteral("KEY_NONE");
    }
    return QStringLiteral("KEY_NONE");
}

QString actionLabel(const KeyAction &action) {
    switch (action.type) {
    case KeyAction::Char:
        return QString("Char('%1')").arg(QChar(action.ch));
    case KeyAction::Space:
        return QStringLiteral("Space");
    case KeyAction::Backspace:
        return QStringLiteral("Backspace");
    case KeyAction::Enter:
        return QStringLiteral("Enter");
    case KeyAction::None:
        return QStringLiteral("None");
    }
    return QStringLiteral("None");
}

} // namespace

InputRouter::InputRouter(QObject *parent)
    : QObject(parent),
      m_layout(RadialLayoutConfig{8, 0.5, 0.5, M_PI / 2.0}) {
}

double InputRouter::clamp01(double value) {
    if (value < 0.0) {
        return 0.0;
    }
    if (value > 1.0) {
        return 1.0;
    }
    return value;
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
    if (type == "touch_down" || type == "touch_move" || type == "touch_up") {
        const double x = clamp01(obj.value("x").toDouble());
        const double y = clamp01(obj.value("y").toDouble());
        Q_ASSERT(x >= 0.0 && x <= 1.0);
        Q_ASSERT(y >= 0.0 && y <= 1.0);
        Logging::log(LogLevel::Debug, "ENGINE",
                     QString("input %1 x=%2 y=%3").arg(type).arg(x, 0, 'f', 3).arg(y, 0, 'f', 3));
        if (type == "touch_down") {
            handleTouchDown(x, y);
        } else if (type == "touch_move") {
            handleTouchMove(x, y);
        } else {
            handleTouchUp(x, y);
        }
    } else if (type == "commit_char") {
        const QString ch = obj.value("char").toString();
        if (!ch.isEmpty()) {
            const QChar value = ch.at(0);
            transitionTo(RouterState::CommitChar, "commit_char");
            if (value == QChar('\n')) {
                m_commit.commitAction("enter");
            } else if (value == QChar('\b')) {
                m_commit.commitAction("backspace");
            } else if (value == QChar(' ')) {
                m_commit.commitAction("space");
            } else {
                m_commit.commitChar(value);
            }
            m_haptics.onCommit();
            transitionTo(RouterState::Idle, "commit_done");
            m_skipCommitOnTouchUp = true;
        }
    } else if (type == "action") {
        handleAction(obj.value("action").toString());
    } else if (type == "ui_show") {
        transitionTo(RouterState::Idle, "ui_show");
    } else if (type == "ui_hide") {
        clearSelection("ui_hide");
    }

    QJsonObject reply;
    reply.insert("ack", true);
    if (type == "touch_move" || type == "touch_down") {
        const bool clearSelection = (m_selectedSector < 0);
        reply.insert("type", "selection");
        reply.insert("sector", m_selectedSector);
        reply.insert("letter", m_selectedKey);
        reply.insert("stage", m_trackingLetter ? "letter" : "group");
        reply.insert("clearSelection", clearSelection);
        if (clearSelection) {
            Logging::log(LogLevel::Debug, "ENGINE", "selection cleared (reply)");
        }
    } else {
        reply.insert("type", "ack");
    }

    return QJsonDocument(reply).toJson(QJsonDocument::Compact);
}

void InputRouter::handleTouchDown(double xNorm, double yNorm) {
    m_lastX = xNorm;
    m_lastY = yNorm;
    m_skipCommitOnTouchUp = false;
    TouchSample sample{xNorm, yNorm, QDateTime::currentMSecsSinceEpoch()};
    m_gestures.onTouchDown(sample);
    transitionTo(RouterState::Hovering, "touch_down");
    updateSelection(xNorm, yNorm);
}

void InputRouter::handleTouchMove(double xNorm, double yNorm) {
    m_lastX = xNorm;
    m_lastY = yNorm;
    TouchSample sample{xNorm, yNorm, QDateTime::currentMSecsSinceEpoch()};
    m_gestures.onTouchMove(sample);
    if (m_state == RouterState::Idle) {
        transitionTo(RouterState::Hovering, "touch_move");
    }
    updateSelection(xNorm, yNorm);
}

void InputRouter::handleTouchUp(double xNorm, double yNorm) {
    TouchSample sample{xNorm, yNorm, QDateTime::currentMSecsSinceEpoch()};
    const SwipeDir swipe = m_gestures.onTouchUp(sample);
    if (m_skipCommitOnTouchUp) {
        m_skipCommitOnTouchUp = false;
        clearSelection("commit_char");
        return;
    }
    if (swipe == SwipeDir::Left) {
        transitionTo(RouterState::CommitChar, "swipe_left");
        m_commit.commitAction("backspace");
        m_haptics.onCommit();
        transitionTo(RouterState::Idle, "commit_done");
        return;
    }
    else if (swipe == SwipeDir::Right) {
        transitionTo(RouterState::CommitChar, "swipe_right");
        m_commit.commitAction("space");
        m_haptics.onCommit();
        transitionTo(RouterState::Idle, "commit_done");
        return;
    }
    else if (swipe == SwipeDir::Down) {
        m_haptics.onCancel();
        clearSelection("swipe_down");
        return;
    }

    updateSelection(xNorm, yNorm);
    if (m_selectedSector < 0) {
        transitionTo(RouterState::Idle, "touch_up_no_selection");
        return;
    }

    const int keyIndex = (m_trackingLetter && m_selectedKey >= 0)
        ? m_selectedKey
        : 0;
    const int keyCount = m_layout.keyCount(m_selectedSector);
    int clampedKeyIndex = 0;
    if (keyCount > 0 && keyIndex >= 0 && keyIndex < keyCount) {
        clampedKeyIndex = keyIndex;
    }
    KeyAction action = KeyAction::make(KeyAction::None);
    QString keyLabel = QStringLiteral("None");
    if (keyCount > 0) {
        const KeyOption &option = m_layout.keyAt(m_selectedSector, clampedKeyIndex);
        action = keyOptionToAction(option);
        keyLabel = option.label;
    }
    Logging::log(LogLevel::Info, "COMMIT",
                 QString("sel=%1:%2 label=%3 action=%4 keycode=%5")
                     .arg(m_selectedSector)
                     .arg(clampedKeyIndex)
                     .arg(keyLabel)
                     .arg(actionLabel(action))
                     .arg(keycodeLabel(action)));
    if (action.type == KeyAction::None) {
        transitionTo(RouterState::Idle, "commit_none");
        return;
    }
    transitionTo(RouterState::CommitChar, "touch_up_commit");
    m_commit.commitAction(action);
    m_haptics.onCommit();
    transitionTo(RouterState::Idle, "commit_done");
}

void InputRouter::handleAction(const QString &actionType) {
    if (actionType == "enter" || actionType == "space" || actionType == "backspace") {
        if (actionType == "enter") {
            transitionTo(RouterState::CommitChar, "action_enter");
        } else if (actionType == "space") {
            transitionTo(RouterState::CommitChar, "action_space");
        } else {
            transitionTo(RouterState::CommitChar, "action_backspace");
        }
        m_commit.commitAction(actionType);
        transitionTo(RouterState::Idle, "commit_done");
    }
    if (actionType == "cancel") {
        m_haptics.onCancel();
        clearSelection("cancel_action");
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
        clearSelection("pad_exit");
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
#ifdef RADIALKB_LEGACY_ROUTER_SM
    m_stateMachine.transitionTo(State::TrackGroup, reason);
#else
    Q_UNUSED(reason);
#endif
}

void InputRouter::enterTrackLetter(const QString &reason) {
    m_trackingLetter = true;
#ifdef RADIALKB_LEGACY_ROUTER_SM
    m_stateMachine.transitionTo(State::TrackLetter, reason);
#else
    Q_UNUSED(reason);
#endif
}

void InputRouter::clearSelection(const char* reason) {
    if (m_selectedSector != -1 || m_selectedKey != -1 || m_trackingLetter) {
        m_selectedSector = -1;
        m_selectedKey = -1;
        m_trackingLetter = false;
        emit selectionChanged(m_selectedSector, m_selectedKey, "group");
        Logging::log(LogLevel::Info, "ENGINE", QString("selection cleared (%1)").arg(reason));
    }
    transitionTo(RouterState::Idle, reason);
}

// ─────────────────────────────────────────────────────────────
// Phase 1.5: InputRouter explicit FSM helpers
// Keep behavior stable; transitions are for clarity + debugging.
// ─────────────────────────────────────────────────────────────
const char* InputRouter::stateName(RouterState s) {
    switch (s) {
    case RouterState::Idle: return "Idle";
    case RouterState::Hovering: return "Hovering";
    case RouterState::CommitChar: return "CommitChar";
    case RouterState::SwipeCapture: return "SwipeCapture";
    }
    return "Unknown";
}

void InputRouter::resetCtx() {
    m_ctx = GestureCtx{};
}

void InputRouter::transitionTo(RouterState next, const char* reason) {
    if (next == m_state) return;
    const auto prev = m_state;
    m_state = next;
    const QString reasonStr = reason ? QString::fromLatin1(reason) : QStringLiteral("");
    Logging::log(LogLevel::Info, "FSM",
                 QString("RouterFSM: %1 -> %2 reason=%3")
                     .arg(stateName(prev))
                     .arg(stateName(next))
                     .arg(reasonStr));
    if (next == RouterState::Idle) {
        resetCtx();
    }
}

} // namespace radialkb
