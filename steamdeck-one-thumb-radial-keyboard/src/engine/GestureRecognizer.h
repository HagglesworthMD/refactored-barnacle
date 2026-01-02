#pragma once

#include <cstdint>

namespace radialkb {

enum class SwipeDir { None, Left, Right, Down, Up };

struct GestureThresholds {
    double minDistanceNorm = 0.12;
    int maxDurationMs = 220;
    double minVelocityNormPerMs = 0.0009;
};

struct TouchSample {
    double x = 0.0;
    double y = 0.0;
    std::int64_t timestampMs = 0;
};

class GestureRecognizer {
public:
    explicit GestureRecognizer(GestureThresholds thresholds = {});

    void onTouchDown(const TouchSample &sample);
    SwipeDir onTouchMove(const TouchSample &sample);
    SwipeDir onTouchUp(const TouchSample &sample);

    const GestureThresholds &thresholds() const;

private:
    SwipeDir classifySwipe(const TouchSample &sample, bool enforceDuration) const;

    GestureThresholds m_thresholds;
    TouchSample m_start;
    bool m_active{false};
};

const char *swipeToString(SwipeDir direction);

} // namespace radialkb
