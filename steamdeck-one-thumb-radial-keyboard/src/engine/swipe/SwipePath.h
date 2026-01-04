#pragma once
#include <QVector>
#include <QPointF>

// Captures a continuous thumb path on the radial surface.
// Phase 2a: data collection only (no scoring, no dictionary).
struct SwipePath {
    QVector<QPointF> points;
    void clear() { points.clear(); }
    void addPoint(const QPointF& p) { points.push_back(p); }
    bool empty() const { return points.isEmpty(); }
};
