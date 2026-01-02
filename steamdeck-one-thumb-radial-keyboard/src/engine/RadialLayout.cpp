#include "RadialLayout.h"

#include <QtMath>

namespace radialkb {

RadialLayout::RadialLayout(RadialLayoutConfig cfg)
    : m_cfg(cfg) {
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
    m_sectors.resize(m_cfg.sectors);
}

int RadialLayout::angleToSector(double xNorm, double yNorm) const {
    const double dx = xNorm - m_cfg.centerX;
    const double dy = yNorm - m_cfg.centerY;

    double angle = std::atan2(dy, dx);
    angle += m_cfg.angleOffsetRad;

    while (angle < 0) {
        angle += 2.0 * M_PI;
    }
    while (angle >= 2.0 * M_PI) {
        angle -= 2.0 * M_PI;
    }

    const double sectorAngle = (2.0 * M_PI) / static_cast<double>(m_cfg.sectors);
    int sector = static_cast<int>(std::floor(angle / sectorAngle));
    if (sector < 0) {
        sector = 0;
    }
    if (sector >= m_cfg.sectors) {
        sector = m_cfg.sectors - 1;
    }
    return sector;
}

} // namespace radialkb
