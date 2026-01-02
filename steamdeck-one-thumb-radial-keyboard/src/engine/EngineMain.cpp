#include <QCoreApplication>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <unistd.h>
#include "InputRouter.h"
#include "Logging.h"

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
