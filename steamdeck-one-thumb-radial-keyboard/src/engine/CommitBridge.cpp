#include "CommitBridge.h"

#include "Logging.h"
#include "UInputKeyboard.h"

#include <linux/input.h>

namespace radialkb {

namespace {

UInputKeyboard &keyboard() {
    static UInputKeyboard instance;
    return instance;
}

} // namespace

void CommitBridge::commitChar(QChar ch) {
    keyboard().sendText(QString(ch));
}

void CommitBridge::commitAction(const QString &action) {
    if (action == "space") {
        keyboard().sendKey(KEY_SPACE);
        return;
    }
    if (action == "backspace") {
        keyboard().sendKey(KEY_BACKSPACE);
        return;
    }
    if (action == "enter") {
        keyboard().sendKey(KEY_ENTER);
        return;
    }
    if (action == "tab") {
        keyboard().sendKey(KEY_TAB);
        return;
    }
    if (action == "escape") {
        keyboard().sendKey(KEY_ESC);
        return;
    }

    Logging::log(LogLevel::Warn, "COMMIT", QString("Unknown action '%1'").arg(action));
}

}
