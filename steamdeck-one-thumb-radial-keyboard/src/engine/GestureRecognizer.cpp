#include "GestureRecognizer.h"

#include <QtMath>

namespace radialkb {

GestureRecognizer::GestureRecognizer(GestureThresholds thresholds)
    : m_thresholds(thresholds) {
}

void GestureRecognizer::onTouchDown(const QPointF &pos) {
    m_active = true;
    m_startPos = pos;
    m_timer.restart();
}

SwipeDirection GestureRecognizer::onTouchMove(const QPointF &pos) {
    if (!m_active) {
        return SwipeDirection::None;
    }
    const QPointF delta = pos - m_startPos;
    const double distance = std::hypot(delta.x(), delta.y());
    const int elapsed = static_cast<int>(m_timer.elapsed());
    if (elapsed == 0) {
        return SwipeDirection::None;
    }
    const double velocity = distance / static_cast<double>(elapsed);
    if (distance >= m_thresholds.minDistancePx && velocity >= m_thresholds.velocityPxPerMs) {
        if (qAbs(delta.x()) > qAbs(delta.y())) {
            return delta.x() > 0 ? SwipeDirection::Right : SwipeDirection::Left;
        }
        if (delta.y() > 0) {
            return SwipeDirection::Down;
        }
    }
    return SwipeDirection::None;
}

SwipeDirection GestureRecognizer::onTouchUp(const QPointF &pos) {
    if (!m_active) {
        return SwipeDirection::None;
    }
    m_active = false;
    const QPointF delta = pos - m_startPos;
    const double distance = std::hypot(delta.x(), delta.y());
    const int elapsed = static_cast<int>(m_timer.elapsed());
    if (elapsed > m_thresholds.maxDurationMs) {
        return SwipeDirection::None;
    }
    if (elapsed == 0) {
        return SwipeDirection::None;
    }
    const double velocity = distance / static_cast<double>(elapsed);
    if (distance >= m_thresholds.minDistancePx && velocity >= m_thresholds.velocityPxPerMs) {
        if (qAbs(delta.x()) > qAbs(delta.y())) {
            return delta.x() > 0 ? SwipeDirection::Right : SwipeDirection::Left;
        }
        if (delta.y() > 0) {
            return SwipeDirection::Down;
        }
    }
    return SwipeDirection::None;
}

const GestureThresholds &GestureRecognizer::thresholds() const {
    return m_thresholds;
}

QString swipeToString(SwipeDirection direction) {
    switch (direction) {
    case SwipeDirection::Left:
        return "Left";
    case SwipeDirection::Right:
        return "Right";
    case SwipeDirection::Down:
        return "Down";
    case SwipeDirection::None:
        return "None";
    }
    return "None";
}

}
