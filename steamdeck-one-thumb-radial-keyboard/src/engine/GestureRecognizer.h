#pragma once

#include <QElapsedTimer>
#include <QPointF>
#include <QString>

namespace radialkb {

enum class SwipeDirection {
    None,
    Left,
    Right,
    Down
};

struct GestureThresholds {
    double minDistancePx = 60.0;
    double velocityPxPerMs = 0.6;
    int maxDurationMs = 220;
};

class GestureRecognizer {
public:
    explicit GestureRecognizer(GestureThresholds thresholds = {});

    void onTouchDown(const QPointF &pos);
    SwipeDirection onTouchUp(const QPointF &pos);
    SwipeDirection onTouchMove(const QPointF &pos);

    const GestureThresholds &thresholds() const;

private:
    GestureThresholds m_thresholds;
    QElapsedTimer m_timer;
    QPointF m_startPos;
    bool m_active{false};
};

QString swipeToString(SwipeDirection direction);

}
