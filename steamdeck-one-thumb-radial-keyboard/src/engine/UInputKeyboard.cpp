#include "UInputKeyboard.h"

#include "Logging.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <QDateTime>

namespace radialkb {

namespace {

struct KeyStroke {
    int key;
    bool shift;
};

bool charToKey(QChar ch, KeyStroke &out) {
    if (ch.isLetter()) {
        bool upper = ch.isUpper();
        QChar lower = ch.toLower();
        char latin = lower.toLatin1();
        if (latin >= 'a' && latin <= 'z') {
            out.key = KEY_A + (latin - 'a');
            out.shift = upper;
            return true;
        }
    }

    if (ch == ' ') {
        out.key = KEY_SPACE;
        out.shift = false;
        return true;
    }
    if (ch == '.') {
        out.key = KEY_DOT;
        out.shift = false;
        return true;
    }
    if (ch == ',') {
        out.key = KEY_COMMA;
        out.shift = false;
        return true;
    }
    if (ch == '?') {
        out.key = KEY_SLASH;
        out.shift = true;
        return true;
    }
    if (ch == '\n' || ch == '\r') {
        out.key = KEY_ENTER;
        out.shift = false;
        return true;
    }

    return false;
}

} // namespace

UInputKeyboard::UInputKeyboard()
    : m_fd(-1)
    , m_available(false)
    , m_errorLogged(false)
    , m_lastInitAttemptMs(0) {}

UInputKeyboard::~UInputKeyboard() {
    if (m_fd >= 0) {
        ioctl(m_fd, UI_DEV_DESTROY);
        close(m_fd);
    }
}

bool UInputKeyboard::available() const {
    return m_available;
}

void UInputKeyboard::sendKey(int linuxKeyCode, bool pressRelease) {
    if (!ensureInitialized()) {
        return;
    }

    emitEvent(EV_KEY, linuxKeyCode, 1);
    emitSync();
    if (pressRelease) {
        emitEvent(EV_KEY, linuxKeyCode, 0);
        emitSync();
    }
}

void UInputKeyboard::sendText(const QString &text) {
    if (!ensureInitialized()) {
        return;
    }

    for (QChar ch : text) {
        KeyStroke stroke{};
        if (!charToKey(ch, stroke)) {
            Logging::log(LogLevel::Warn, "COMMIT",
                         QString("Unsupported character for uinput: '%1'").arg(ch));
            continue;
        }

        if (stroke.shift) {
            emitEvent(EV_KEY, KEY_LEFTSHIFT, 1);
            emitSync();
        }

        emitEvent(EV_KEY, stroke.key, 1);
        emitSync();
        emitEvent(EV_KEY, stroke.key, 0);
        emitSync();

        if (stroke.shift) {
            emitEvent(EV_KEY, KEY_LEFTSHIFT, 0);
            emitSync();
        }
    }
}

bool UInputKeyboard::ensureInitialized() {
    if (m_available) {
        return true;
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastInitAttemptMs < 2000) {
        return false;
    }
    m_lastInitAttemptMs = now;

    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }

    m_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (m_fd < 0) {
        logUnavailable(QString("Unable to open /dev/uinput (%1). Try: modprobe uinput; add user to input group.")
                           .arg(QString::fromLocal8Bit(strerror(errno))));
        return false;
    }

    if (ioctl(m_fd, UI_SET_EVBIT, EV_KEY) < 0 || ioctl(m_fd, UI_SET_EVBIT, EV_SYN) < 0) {
        logUnavailable(QString("Failed to set uinput event bits (%1).")
                           .arg(QString::fromLocal8Bit(strerror(errno))));
        return false;
    }

    for (int code = KEY_A; code <= KEY_Z; ++code) {
        ioctl(m_fd, UI_SET_KEYBIT, code);
    }
    ioctl(m_fd, UI_SET_KEYBIT, KEY_SPACE);
    ioctl(m_fd, UI_SET_KEYBIT, KEY_ENTER);
    ioctl(m_fd, UI_SET_KEYBIT, KEY_BACKSPACE);
    ioctl(m_fd, UI_SET_KEYBIT, KEY_DOT);
    ioctl(m_fd, UI_SET_KEYBIT, KEY_COMMA);
    ioctl(m_fd, UI_SET_KEYBIT, KEY_SLASH);
    ioctl(m_fd, UI_SET_KEYBIT, KEY_LEFTSHIFT);
    ioctl(m_fd, UI_SET_KEYBIT, KEY_TAB);
    ioctl(m_fd, UI_SET_KEYBIT, KEY_ESC);

    uinput_setup setup{};
    setup.id.bustype = BUS_USB;
    setup.id.vendor = 0x1234;
    setup.id.product = 0x5678;
    setup.id.version = 1;
    strncpy(setup.name, "radialkb-uinput", sizeof(setup.name) - 1);

    if (ioctl(m_fd, UI_DEV_SETUP, &setup) < 0) {
        logUnavailable(QString("Failed to setup uinput device (%1).")
                           .arg(QString::fromLocal8Bit(strerror(errno))));
        return false;
    }
    if (ioctl(m_fd, UI_DEV_CREATE) < 0) {
        logUnavailable(QString("Failed to create uinput device (%1).")
                           .arg(QString::fromLocal8Bit(strerror(errno))));
        return false;
    }

    m_available = true;
    Logging::log(LogLevel::Info, "COMMIT", "uinput keyboard initialized.");
    return true;
}

void UInputKeyboard::emitEvent(std::uint16_t type, std::uint16_t code, std::int32_t value) {
    if (m_fd < 0) {
        return;
    }
    input_event event{};
    event.type = type;
    event.code = code;
    event.value = value;
    const ssize_t written = write(m_fd, &event, sizeof(event));
    if (written != static_cast<ssize_t>(sizeof(event))) {
        logUnavailable(QString("uinput write failed (%1).").arg(QString::fromLocal8Bit(strerror(errno))));
    }
}

void UInputKeyboard::emitSync() {
    emitEvent(EV_SYN, SYN_REPORT, 0);
}

void UInputKeyboard::logUnavailable(const QString &reason) {
    if (!m_errorLogged) {
        Logging::log(LogLevel::Error, "COMMIT", reason);
        m_errorLogged = true;
    }
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
    m_available = false;
}

} // namespace radialkb
