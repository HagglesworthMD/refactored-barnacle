#pragma once

#include <QString>

namespace radialkb {

enum class LogLevel { Debug, Info, Warn, Error };

class Logging {
public:
    static void init(const QString &appTag);
    static void log(LogLevel lvl, const QString &component, const QString &msg);
};

} // namespace radialkb
