#pragma once

#include <QString>
#include <QtGlobal>
#include <cstdint>

namespace radialkb {

class UInputKeyboard {
public:
    UInputKeyboard();
    ~UInputKeyboard();

    bool available() const;
    void sendKey(int linuxKeyCode, bool pressRelease = true);
    void sendText(const QString &text);

private:
    bool ensureInitialized();
    void emitEvent(std::uint16_t type, std::uint16_t code, std::int32_t value);
    void emitSync();
    void logUnavailable(const QString &reason);

    int m_fd;
    bool m_available;
    bool m_errorLogged;
    qint64 m_lastInitAttemptMs;
};

} // namespace radialkb
