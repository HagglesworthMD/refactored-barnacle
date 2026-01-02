#pragma once

#include <QObject>
#include <QPointF>
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
    void handleTouchDown(const QPointF &pos);
    void handleTouchMove(const QPointF &pos);
    void handleTouchUp(const QPointF &pos);
    void handleAction(const QString &actionType);

    StateMachine m_stateMachine;
    RadialLayout m_layout;
    GestureRecognizer m_gestures;
    CommitBridge m_commit;
    Haptics m_haptics;
    int m_selectedSector{-1};
    QPointF m_lastPos;
};

}
