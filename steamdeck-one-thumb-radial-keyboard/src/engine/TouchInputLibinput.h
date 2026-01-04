#pragma once

#include <QObject>
#include <QSocketNotifier>
#include <QString>

struct libinput;
struct libinput_device;

namespace radialkb {

// TouchInputLibinput: libinput PATH backend for touchscreen input.
// Uses QSocketNotifier to integrate with Qt event loop.
// Emits normalized 0..1 coordinates based on physical device dimensions.
class TouchInputLibinput : public QObject {
    Q_OBJECT
public:
    explicit TouchInputLibinput(const QString &devicePath = QStringLiteral("/dev/input/event15"),
                                 QObject *parent = nullptr);
    ~TouchInputLibinput() override;

    bool start();
    void stop();
    bool isActive() const { return m_active; }

    // Device physical dimensions in mm (Steam Deck touchscreen defaults)
    void setDeviceDimensions(double widthMm, double heightMm);
    double widthMm() const { return m_widthMm; }
    double heightMm() const { return m_heightMm; }

signals:
    void touchDown(int slot, double xNorm, double yNorm);
    void touchMove(int slot, double xNorm, double yNorm);
    void touchUp(int slot);

private slots:
    void onReadyRead();

private:
    QString m_devicePath;
    libinput *m_li = nullptr;
    libinput_device *m_device = nullptr;
    QSocketNotifier *m_notifier = nullptr;
    bool m_active = false;

    // Device dimensions for normalization (Steam Deck FTS3528 defaults)
    double m_widthMm = 267.0;
    double m_heightMm = 142.0;

    static int openRestricted(const char *path, int flags, void *userData);
    static void closeRestricted(int fd, void *userData);
};

} // namespace radialkb
