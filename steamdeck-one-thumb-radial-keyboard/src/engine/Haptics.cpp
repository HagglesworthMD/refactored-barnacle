#include "Haptics.h"

#include "Logging.h"

namespace radialkb {

void Haptics::onSelectionChange() {
    Logging::log(LogLevel::Debug, "ENGINE", "haptics selection (stub)");
}

void Haptics::onCommit() {
    Logging::log(LogLevel::Debug, "ENGINE", "haptics commit (stub)");
}

void Haptics::onCancel() {
    Logging::log(LogLevel::Debug, "ENGINE", "haptics cancel (stub)");
}

}
