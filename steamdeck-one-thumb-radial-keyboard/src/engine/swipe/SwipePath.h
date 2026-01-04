#pragma once
#include <QVector>
#include <QPointF>
#include <QLineF>
#include <QtMath>

// Captures a continuous thumb path on the radial surface.
// Phase 2a: data collection only (no scoring, no dictionary).
struct SwipePath {
    QVector<QPointF> points;
    void clear() { points.clear(); }
    void addPoint(const QPointF& p) { points.push_back(p); }
    bool empty() const { return points.isEmpty(); }

    // Total Euclidean path length
    qreal length() const {
        if (points.size() < 2) return 0.0;
        qreal sum = 0.0;
        for (int i = 1; i < points.size(); ++i)
            sum += QLineF(points[i-1], points[i]).length();
        return sum;
    }

    // Maximum radius from origin (0,0)
    qreal maxRadius() const {
        qreal r = 0.0;
        for (const auto& p : points)
            r = qMax(r, qSqrt(p.x()*p.x() + p.y()*p.y()));
        return r;
    }

    // Rough direction change count (angle deltas)
    int directionChanges(qreal thresholdDeg = 30.0) const {
        if (points.size() < 3) return 0;
        int changes = 0;
        for (int i = 2; i < points.size(); ++i) {
            QLineF a(points[i-2], points[i-1]);
            QLineF b(points[i-1], points[i]);
            if (qAbs(a.angleTo(b)) > thresholdDeg)
                ++changes;
        }
        return changes;
    }
};
