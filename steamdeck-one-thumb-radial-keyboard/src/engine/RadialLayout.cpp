#include "RadialLayout.h"

#include <QtMath>
#include <cmath>

namespace radialkb {

RadialLayout::RadialLayout(RadialLayoutConfig cfg)
    : m_cfg(cfg) {
    m_sectors = {
        {"ETAO", {{"E", QChar('e'), ""}, {"T", QChar('t'), ""}, {"A", QChar('a'), ""}, {"O", QChar('o'), ""}}},
        {"INSH", {{"I", QChar('i'), ""}, {"N", QChar('n'), ""}, {"S", QChar('s'), ""}, {"H", QChar('h'), ""}}},
        {"RDLU", {{"R", QChar('r'), ""}, {"D", QChar('d'), ""}, {"L", QChar('l'), ""}, {"U", QChar('u'), ""}}},
        {"CMFW", {{"C", QChar('c'), ""}, {"M", QChar('m'), ""}, {"F", QChar('f'), ""}, {"W", QChar('w'), ""}}},
        {"GPBY", {{"G", QChar('g'), ""}, {"P", QChar('p'), ""}, {"B", QChar('b'), ""}, {"Y", QChar('y'), ""}}},
        {"VKJX", {{"V", QChar('v'), ""}, {"K", QChar('k'), ""}, {"J", QChar('j'), ""}, {"X", QChar('x'), ""}}},
        {"QZ.,?", {{"Q", QChar('q'), ""}, {"Z", QChar('z'), ""}, {".", QChar('.'), ""}, {",", QChar(','), ""}, {"?", QChar('?'), ""}}},
        {"CMD", {{"␠", QChar(), "space"}, {"⌫", QChar(), "backspace"}, {"↵", QChar(), "enter"}}}
    };
    const int baseSize = m_sectors.size();
    if (m_cfg.sectors > baseSize) {
        for (int i = baseSize; i < m_cfg.sectors; ++i) {
            const int source = i % baseSize;
            m_sectors.push_back({QString("EXT%1").arg(i + 1), m_sectors.at(source).keys});
        }
    }
    m_sectors.resize(m_cfg.sectors);
}

double RadialLayout::angleForPoint(double xNorm, double yNorm) const {
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
    return angle;
}

double RadialLayout::radiusForPoint(double xNorm, double yNorm) const {
    const double dx = xNorm - m_cfg.centerX;
    const double dy = yNorm - m_cfg.centerY;
    return std::hypot(dx, dy);
}

int RadialLayout::angleToSector(double xNorm, double yNorm) const {
    const double angle = angleForPoint(xNorm, yNorm);
    return angleToSector(angle);
}

int RadialLayout::angleToSector(double angleRad) const {
    const double sectorAngle = (2.0 * M_PI) / static_cast<double>(m_cfg.sectors);
    int sector = static_cast<int>(std::floor(angleRad / sectorAngle));
    if (sector < 0) {
        sector = 0;
    }
    if (sector >= m_cfg.sectors) {
        sector = m_cfg.sectors - 1;
    }
    return sector;
}

namespace {
double angularDistance(double a, double b) {
    double diff = std::fmod(std::abs(a - b), 2.0 * M_PI);
    if (diff > M_PI) {
        diff = 2.0 * M_PI - diff;
    }
    return diff;
}
}

int RadialLayout::angleToSectorWithHysteresis(double angleRad, int previousSector, double hysteresisRad) const {
    const int rawSector = angleToSector(angleRad);
    if (previousSector < 0 || previousSector >= m_cfg.sectors) {
        return rawSector;
    }
    if (rawSector == previousSector) {
        return rawSector;
    }
    const double sectorAngle = (2.0 * M_PI) / static_cast<double>(m_cfg.sectors);
    const double startAngle = sectorAngle * static_cast<double>(previousSector);
    const double endAngle = startAngle + sectorAngle;
    if (angularDistance(angleRad, startAngle) < hysteresisRad ||
        angularDistance(angleRad, endAngle) < hysteresisRad) {
        return previousSector;
    }
    return rawSector;
}

int RadialLayout::angleToKeyIndex(double angleRad, int sectorIndex) const {
    if (sectorIndex < 0 || sectorIndex >= m_sectors.size()) {
        return -1;
    }
    const int keyCount = m_sectors.at(sectorIndex).keys.size();
    if (keyCount <= 0) {
        return -1;
    }
    const double sectorAngle = (2.0 * M_PI) / static_cast<double>(m_cfg.sectors);
    const double startAngle = sectorAngle * static_cast<double>(sectorIndex);
    double localAngle = angleRad - startAngle;
    while (localAngle < 0) {
        localAngle += 2.0 * M_PI;
    }
    while (localAngle >= 2.0 * M_PI) {
        localAngle -= 2.0 * M_PI;
    }
    const double keyAngle = sectorAngle / static_cast<double>(keyCount);
    int index = static_cast<int>(std::floor(localAngle / keyAngle));
    if (index < 0) {
        index = 0;
    }
    if (index >= keyCount) {
        index = keyCount - 1;
    }
    return index;
}

int RadialLayout::angleToKeyIndexWithHysteresis(double angleRad, int sectorIndex, int previousIndex, double hysteresisRad) const {
    const int rawIndex = angleToKeyIndex(angleRad, sectorIndex);
    if (sectorIndex < 0 || sectorIndex >= m_sectors.size()) {
        return rawIndex;
    }
    const int keyCount = m_sectors.at(sectorIndex).keys.size();
    if (previousIndex < 0 || previousIndex >= keyCount) {
        return rawIndex;
    }
    if (rawIndex == previousIndex) {
        return rawIndex;
    }
    const double sectorAngle = (2.0 * M_PI) / static_cast<double>(m_cfg.sectors);
    const double startAngle = sectorAngle * static_cast<double>(sectorIndex);
    const double keyAngle = sectorAngle / static_cast<double>(keyCount);
    const double keyStart = startAngle + keyAngle * static_cast<double>(previousIndex);
    const double keyEnd = keyStart + keyAngle;
    if (angularDistance(angleRad, keyStart) < hysteresisRad ||
        angularDistance(angleRad, keyEnd) < hysteresisRad) {
        return previousIndex;
    }
    return rawIndex;
}

int RadialLayout::keyCount(int sectorIndex) const {
    if (sectorIndex < 0 || sectorIndex >= m_sectors.size()) {
        return 0;
    }
    return m_sectors.at(sectorIndex).keys.size();
}

const KeyOption &RadialLayout::keyAt(int sectorIndex, int keyIndex) const {
    return m_sectors.at(sectorIndex).keys.at(keyIndex);
}

const KeyOption &RadialLayout::defaultKey(int sectorIndex) const {
    return m_sectors.at(sectorIndex).keys.first();
}

} // namespace radialkb
