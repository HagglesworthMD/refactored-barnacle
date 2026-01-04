#include "TouchInputLibinput.h"
#include "Logging.h"

#include <libinput.h>
#include <fcntl.h>
#include <unistd.h>

namespace radialkb {

namespace {

const libinput_interface kInterface = {
    .open_restricted = TouchInputLibinput::openRestricted,
    .close_restricted = TouchInputLibinput::closeRestricted,
};

} // namespace

TouchInputLibinput::TouchInputLibinput(const QString &devicePath, QObject *parent)
    : QObject(parent)
    , m_devicePath(devicePath)
{
}

TouchInputLibinput::~TouchInputLibinput()
{
    stop();
}

int TouchInputLibinput::openRestricted(const char *path, int flags, void *userData)
{
    Q_UNUSED(userData);
    int fd = open(path, flags);
    if (fd < 0) {
        Logging::log(LogLevel::Error, "TOUCH",
                     QString("Failed to open device %1: %2").arg(path).arg(strerror(errno)));
    }
    return fd;
}

void TouchInputLibinput::closeRestricted(int fd, void *userData)
{
    Q_UNUSED(userData);
    close(fd);
}

bool TouchInputLibinput::start()
{
    if (m_active) {
        return true;
    }

    m_li = libinput_path_create_context(&kInterface, this);
    if (!m_li) {
        Logging::log(LogLevel::Error, "TOUCH", "Failed to create libinput context");
        return false;
    }

    m_device = libinput_path_add_device(m_li, m_devicePath.toLocal8Bit().constData());
    if (!m_device) {
        Logging::log(LogLevel::Error, "TOUCH",
                     QString("Failed to add device: %1").arg(m_devicePath));
        libinput_unref(m_li);
        m_li = nullptr;
        return false;
    }

    int fd = libinput_get_fd(m_li);
    m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &TouchInputLibinput::onReadyRead);

    m_active = true;
    Logging::log(LogLevel::Info, "TOUCH",
                 QString("Started touchscreen input on %1 (%2x%3 mm)")
                     .arg(m_devicePath)
                     .arg(m_widthMm)
                     .arg(m_heightMm));
    return true;
}

void TouchInputLibinput::stop()
{
    if (!m_active) {
        return;
    }

    m_active = false;

    if (m_notifier) {
        m_notifier->setEnabled(false);
        delete m_notifier;
        m_notifier = nullptr;
    }

    if (m_device) {
        libinput_path_remove_device(m_device);
        m_device = nullptr;
    }

    if (m_li) {
        libinput_unref(m_li);
        m_li = nullptr;
    }

    Logging::log(LogLevel::Info, "TOUCH", "Stopped touchscreen input");
}

void TouchInputLibinput::setDeviceDimensions(double widthMm, double heightMm)
{
    m_widthMm = widthMm;
    m_heightMm = heightMm;
}

void TouchInputLibinput::onReadyRead()
{
    if (!m_li) {
        return;
    }

    libinput_dispatch(m_li);

    libinput_event *ev;
    while ((ev = libinput_get_event(m_li)) != nullptr) {
        const auto type = libinput_event_get_type(ev);

        if (type == LIBINPUT_EVENT_TOUCH_DOWN ||
            type == LIBINPUT_EVENT_TOUCH_MOTION ||
            type == LIBINPUT_EVENT_TOUCH_UP) {

            auto *touch = libinput_event_get_touch_event(ev);
            const int slot = libinput_event_touch_get_slot(touch);

            if (type == LIBINPUT_EVENT_TOUCH_UP) {
                Logging::log(LogLevel::Debug, "TOUCH",
                             QString("touch_up slot=%1").arg(slot));
                emit touchUp(slot);
            } else {
                // Get coordinates in mm
                const double xMm = libinput_event_touch_get_x(touch);
                const double yMm = libinput_event_touch_get_y(touch);

                // Normalize to 0..1 based on device dimensions
                const double xNorm = (m_widthMm > 0.0) ? (xMm / m_widthMm) : 0.0;
                const double yNorm = (m_heightMm > 0.0) ? (yMm / m_heightMm) : 0.0;

                // Clamp to valid range
                const double xClamped = qBound(0.0, xNorm, 1.0);
                const double yClamped = qBound(0.0, yNorm, 1.0);

                if (type == LIBINPUT_EVENT_TOUCH_DOWN) {
                    Logging::log(LogLevel::Debug, "TOUCH",
                                 QString("touch_down slot=%1 x=%2 y=%3 (mm: %4,%5)")
                                     .arg(slot)
                                     .arg(xClamped, 0, 'f', 3)
                                     .arg(yClamped, 0, 'f', 3)
                                     .arg(xMm, 0, 'f', 1)
                                     .arg(yMm, 0, 'f', 1));
                    emit touchDown(slot, xClamped, yClamped);
                } else {
                    Logging::log(LogLevel::Debug, "TOUCH",
                                 QString("touch_move slot=%1 x=%2 y=%3")
                                     .arg(slot)
                                     .arg(xClamped, 0, 'f', 3)
                                     .arg(yClamped, 0, 'f', 3));
                    emit touchMove(slot, xClamped, yClamped);
                }
            }
        }

        libinput_event_destroy(ev);
    }
}

} // namespace radialkb
