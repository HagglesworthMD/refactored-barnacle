#include <QCoreApplication>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <unistd.h>
#include <memory>
#include "InputRouter.h"
#include "Logging.h"

#ifdef RADIALKB_HAS_LIBINPUT
#include "TouchInputLibinput.h"
#endif

using namespace radialkb;

namespace {
QString socketPath() {
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (!runtimeDir.isEmpty()) {
        return runtimeDir + "/radialkb.sock";
    }
    return QString("/tmp/radialkb-%1.sock").arg(getuid());
}
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QCoreApplication::setOrganizationName("radialkb");
    QCoreApplication::setOrganizationDomain("radialkb.local");
    QCoreApplication::setApplicationName("radialkb-engine");
    Logging::init("ENGINE");

    const QString path = socketPath();
    if (QFile::exists(path) && !QLocalServer::removeServer(path)) {
        Logging::log(LogLevel::Warn, "ENGINE", QString("failed to remove stale socket: %1").arg(path));
    }
    QLocalServer server;
    if (!server.listen(path)) {
        Logging::log(LogLevel::Error, "ENGINE", QString("failed to listen: %1").arg(server.errorString()));
        return 1;
    }

    InputRouter router;

#ifdef RADIALKB_HAS_LIBINPUT
    // Touchscreen input via libinput (opt-in via RADIALKB_INPUT=touch)
    std::unique_ptr<TouchInputLibinput> touchInput;
    const QString inputMode = qEnvironmentVariable("RADIALKB_INPUT");
    if (inputMode == QStringLiteral("touch")) {
        const QString devicePath = qEnvironmentVariable("RADIALKB_TOUCH_DEVICE",
                                                         QStringLiteral("/dev/input/event15"));
        touchInput = std::make_unique<TouchInputLibinput>(devicePath);

        // Forward touch events to router as JSON messages
        QObject::connect(touchInput.get(), &TouchInputLibinput::touchDown,
            [&router](int slot, double x, double y) {
                Q_UNUSED(slot);
                router.handleMessage(QString(R"({"type":"touch_down","x":%1,"y":%2})")
                    .arg(x, 0, 'f', 6).arg(y, 0, 'f', 6));
            });
        QObject::connect(touchInput.get(), &TouchInputLibinput::touchMove,
            [&router](int slot, double x, double y) {
                Q_UNUSED(slot);
                router.handleMessage(QString(R"({"type":"touch_move","x":%1,"y":%2})")
                    .arg(x, 0, 'f', 6).arg(y, 0, 'f', 6));
            });
        QObject::connect(touchInput.get(), &TouchInputLibinput::touchUp,
            [&router](int slot) {
                Q_UNUSED(slot);
                router.handleMessage(QStringLiteral(R"({"type":"touch_up","x":0,"y":0})"));
            });

        if (touchInput->start()) {
            Logging::log(LogLevel::Info, "ENGINE", "Touchscreen input mode enabled");
        } else {
            Logging::log(LogLevel::Error, "ENGINE", "Failed to start touchscreen input");
            touchInput.reset();
        }
    }
#endif

    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        auto *socket = server.nextPendingConnection();
        Logging::log(LogLevel::Info, "ENGINE", "ui connected");
        QObject::connect(socket, &QLocalSocket::readyRead, [socket, &router]() {
            while (socket->canReadLine()) {
                const QByteArray line = socket->readLine().trimmed();
                if (line.isEmpty()) {
                    continue;
                }
                const QString response = router.handleMessage(QString::fromUtf8(line));
                socket->write(response.toUtf8());
                socket->write("\n");
            }
        });
        QObject::connect(socket, &QLocalSocket::disconnected, [socket]() {
            socket->deleteLater();
        });
    });

    Logging::log(LogLevel::Info, "ENGINE", "engine ready");
    return app.exec();
}
