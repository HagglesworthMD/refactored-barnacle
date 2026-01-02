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

struct Sector {
    QString label;
    QChar primaryChar;
};

class RadialLayout {
public:
    explicit RadialLayout(RadialLayoutConfig cfg = {});

    int sectors() const { return m_cfg.sectors; }
    const QVector<Sector> &sectorList() const { return m_sectors; }

    int angleToSector(double xNorm, double yNorm) const;

private:
    RadialLayoutConfig m_cfg;
    QVector<Sector> m_sectors;
};

} // namespace radialkb
