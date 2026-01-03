#pragma once

#include <QObject>
#include <QString>
#include <QPointF>
#include <QtGlobal>

#include "CommitBridge.h"
#include "GestureRecognizer.h"
#include "Haptics.h"
#include "RadialLayout.h"
#include "StateMachine.h"

namespace radialkb {

class InputRouter : public QObject {
    Q_OBJECT
public:
    // Phase 1.5: explicit input finite-state machine (FSM)
    enum class RouterState {
        Idle,
        Hovering,
        Touching,
        Dragging,
        Swiping,
        Cancelled,
    };
    static const char* stateName(RouterState s);
    RouterState state() const { return m_state; }

    explicit InputRouter(QObject *parent = nullptr);

    QString handleMessage(const QString &line);

signals:
    void selectionChanged(int sectorIndex, int keyIndex, const QString &stage);

private:
    // FSM per-gesture context. Keep it small; do not change thresholds/semantics here.
    struct GestureCtx {
        bool active = false;
        QPointF startPos;
        QPointF lastPos;
        qint64 startMs = 0;
        double accumDist = 0.0;
        int currentSector = -1;
    };

    void resetCtx();
    void transitionTo(RouterState next, const char* reason);

    RouterState m_state = RouterState::Idle;
    GestureCtx m_ctx;

    void handleTouchDown(double xNorm, double yNorm);
    void handleTouchMove(double xNorm, double yNorm);
    void handleTouchUp(double xNorm, double yNorm);
    void handleAction(const QString &actionType);
    void updateSelection(double xNorm, double yNorm);
    void enterTrackGroup(const QString &reason);
    void enterTrackLetter(const QString &reason);

    StateMachine m_stateMachine;
    RadialLayout m_layout;
    GestureRecognizer m_gestures;
    CommitBridge m_commit;
    Haptics m_haptics;
    int m_selectedSector{-1};
    int m_selectedKey{-1};
    bool m_trackingLetter{false};
    double m_lastX{0.0};
    double m_lastY{0.0};
};

}
