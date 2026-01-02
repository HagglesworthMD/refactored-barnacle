#pragma once

#include <QString>
#include <QVector>

namespace radialkb {

struct Sector {
    QString label;
    QChar primaryChar;
};

class RadialLayout {
public:
    explicit RadialLayout(int sectors = 8);

    int sectorCount() const;
    const QVector<Sector> &sectors() const;

    int angleToSector(double angleRadians) const;

private:
    int m_sectorCount;
    QVector<Sector> m_sectors;
};

}
