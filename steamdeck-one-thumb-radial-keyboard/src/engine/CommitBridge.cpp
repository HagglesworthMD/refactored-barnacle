#include "CommitBridge.h"

#include "Logging.h"

namespace radialkb {

void CommitBridge::commitChar(QChar ch) {
    logWithTag("[COMMIT]", QString("commit_char '%1' (stub)").arg(ch));
}

void CommitBridge::commitAction(const QString &action) {
    logWithTag("[COMMIT]", QString("action '%1' (stub)").arg(action));
}

}
