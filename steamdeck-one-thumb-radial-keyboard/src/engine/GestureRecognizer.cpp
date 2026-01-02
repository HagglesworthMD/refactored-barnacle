#include "GestureRecognizer.h"

#include <cmath>

namespace radialkb {

GestureRecognizer::GestureRecognizer(GestureThresholds thresholds)
    : m_thresholds(thresholds) {
}

void GestureRecognizer::onTouchDown(const TouchSample &sample) {
    m_active = true;
    m_start = sample;
}

SwipeDir GestureRecognizer::onTouchMove(const TouchSample &sample) {
    if (!m_active) {
        return SwipeDir::None;
    }
    return classifySwipe(sample, false);
}

SwipeDir GestureRecognizer::onTouchUp(const TouchSample &sample) {
    if (!m_active) {
        return SwipeDir::None;
    }
    m_active = false;
    return classifySwipe(sample, true);
}

const GestureThresholds &GestureRecognizer::thresholds() const {
    return m_thresholds;
}

SwipeDir GestureRecognizer::classifySwipe(const TouchSample &sample, bool enforceDuration) const {
    const double dx = sample.x - m_start.x;
    const double dy = sample.y - m_start.y;
    const double distance = std::hypot(dx, dy);
    const double durationMs = static_cast<double>(sample.timestampMs - m_start.timestampMs);

    if (durationMs <= 0.0) {
        return SwipeDir::None;
    }

    if (enforceDuration && durationMs > m_thresholds.maxDurationMs) {
        return SwipeDir::None;
    }

    const double velocity = distance / durationMs;
    if (distance < m_thresholds.minDistanceNorm || velocity < m_thresholds.minVelocityNormPerMs) {
        return SwipeDir::None;
    }

    if (std::abs(dx) > std::abs(dy)) {
        return dx > 0 ? SwipeDir::Right : SwipeDir::Left;
    }
    return dy > 0 ? SwipeDir::Down : SwipeDir::Up;
}

const char *swipeToString(SwipeDir direction) {
    switch (direction) {
    case SwipeDir::Left:
        return "Left";
    case SwipeDir::Right:
        return "Right";
    case SwipeDir::Down:
        return "Down";
    case SwipeDir::Up:
        return "Up";
    case SwipeDir::None:
        return "None";
    }
    return "None";
}

} // namespace radialkb
