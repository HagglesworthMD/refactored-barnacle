#include "Logging.h"

#include <QDateTime>
#include <QTextStream>

namespace radialkb {

static QString g_appTag = "RADIALKB";

static const char *levelStr(LogLevel lvl) {
    switch (lvl) {
    case LogLevel::Debug:
        return "D";
    case LogLevel::Info:
        return "I";
    case LogLevel::Warn:
        return "W";
    case LogLevel::Error:
        return "E";
    }
    return "I";
}

void Logging::init(const QString &appTag) {
    g_appTag = appTag;
}

void Logging::log(LogLevel lvl, const QString &component, const QString &msg) {
    const QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QTextStream out(stdout);
    out << levelStr(lvl) << " " << ts << " [" << g_appTag << "][" << component << "] " << msg
        << "\n";
    out.flush();
}

} // namespace radialkb
