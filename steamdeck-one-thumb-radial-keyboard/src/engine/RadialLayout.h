#pragma once

#include <QChar>
#include <QString>
#include <QVector>

namespace radialkb {

struct RadialLayoutConfig {
    int sectors = 8;
    double centerX = 0.5;
    double centerY = 0.5;
    double angleOffsetRad = 0.0;
};

struct KeyOption {
    QString label;
    QChar ch;
    QString action;

    bool isAction() const { return !action.isEmpty(); }
};

struct Sector {
    QString label;
    QVector<KeyOption> keys;
};

class RadialLayout {
public:
    explicit RadialLayout(RadialLayoutConfig cfg = {});

    int sectors() const { return m_cfg.sectors; }
    const QVector<Sector> &sectorList() const { return m_sectors; }

    double angleForPoint(double xNorm, double yNorm) const;
    double radiusForPoint(double xNorm, double yNorm) const;

    int angleToSector(double xNorm, double yNorm) const;
    int angleToSector(double angleRad) const;
    int angleToSectorWithHysteresis(double angleRad, int previousSector, double hysteresisRad) const;

    int angleToKeyIndex(double angleRad, int sectorIndex) const;
    int angleToKeyIndexWithHysteresis(double angleRad, int sectorIndex, int previousIndex, double hysteresisRad) const;

    int keyCount(int sectorIndex) const;
    const KeyOption &keyAt(int sectorIndex, int keyIndex) const;
    const KeyOption &defaultKey(int sectorIndex) const;

private:
    RadialLayoutConfig m_cfg;
    QVector<Sector> m_sectors;
};

} // namespace radialkb
