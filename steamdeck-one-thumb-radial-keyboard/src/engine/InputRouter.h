#pragma once

#include <QObject>
#include <QString>

#include "CommitBridge.h"
#include "GestureRecognizer.h"
#include "Haptics.h"
#include "RadialLayout.h"
#include "StateMachine.h"

namespace radialkb {

class InputRouter : public QObject {
    Q_OBJECT
public:
    explicit InputRouter(QObject *parent = nullptr);

    QString handleMessage(const QString &line);

signals:
    void selectionChanged(int sectorIndex);

private:
    void handleTouchDown(double xNorm, double yNorm);
    void handleTouchMove(double xNorm, double yNorm);
    void handleTouchUp(double xNorm, double yNorm);
    void handleAction(const QString &actionType);

    StateMachine m_stateMachine;
    RadialLayout m_layout;
    GestureRecognizer m_gestures;
    CommitBridge m_commit;
    Haptics m_haptics;
    int m_selectedSector{-1};
    double m_lastX{0.0};
    double m_lastY{0.0};
};

}
