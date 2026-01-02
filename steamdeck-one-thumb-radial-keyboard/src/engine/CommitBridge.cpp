#include "CommitBridge.h"

#include "Logging.h"

namespace radialkb {

void CommitBridge::commitChar(QChar ch) {
    Logging::log(LogLevel::Info, "COMMIT", QString("commit_text '%1' (stub)").arg(ch));
}

void CommitBridge::commitAction(const QString &action) {
    Logging::log(LogLevel::Info, "COMMIT", QString("commit_key '%1' (stub)").arg(action));
}

}
