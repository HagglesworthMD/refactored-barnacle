#include "RadialLayout.h"

#include <QtMath>

namespace radialkb {

RadialLayout::RadialLayout(int sectors)
    : m_sectorCount(sectors) {
    m_sectors = {
        {"ETA", QChar('e')},
        {"OIN", QChar('t')},
        {"SHR", QChar('a')},
        {"LUD", QChar('o')},
        {"GCM", QChar('i')},
        {"PFY", QChar('n')},
        {"WBV", QChar('s')},
        {"KJX", QChar('r')}
    };
    m_sectors.resize(m_sectorCount);
}

int RadialLayout::sectorCount() const {
    return m_sectorCount;
}

const QVector<Sector> &RadialLayout::sectors() const {
    return m_sectors;
}

int RadialLayout::angleToSector(double angleRadians) const {
    double angle = angleRadians;
    if (angle < 0) {
        angle += 2 * M_PI;
    }
    const double sectorSize = (2 * M_PI) / m_sectorCount;
    int index = static_cast<int>(angle / sectorSize);
    if (index < 0) {
        index = 0;
    }
    if (index >= m_sectorCount) {
        index = m_sectorCount - 1;
    }
    return index;
}

}
