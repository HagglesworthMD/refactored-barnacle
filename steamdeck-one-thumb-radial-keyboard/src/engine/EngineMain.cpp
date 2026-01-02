#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

#include "InputRouter.h"
#include "Logging.h"

using namespace radialkb;

namespace {
const char *kSocketName = "/tmp/radialkb.sock";
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QLocalServer::removeServer(kSocketName);
    QLocalServer server;
    if (!server.listen(kSocketName)) {
        logWithTag("[ENGINE]", QString("failed to listen: %1").arg(server.errorString()));
        return 1;
    }

    InputRouter router;

    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        auto *socket = server.nextPendingConnection();
        logWithTag("[ENGINE]", "ui connected");
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

    logWithTag("[ENGINE]", "engine ready");
    return app.exec();
}
