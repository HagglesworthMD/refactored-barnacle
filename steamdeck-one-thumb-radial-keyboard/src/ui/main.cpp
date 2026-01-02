#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <unistd.h>

class UiBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
public:
    explicit UiBridge(QObject *parent = nullptr)
        : QObject(parent) {
        connect(&m_socket, &QLocalSocket::connected, this, &UiBridge::connectedChanged);
        connect(&m_socket, &QLocalSocket::disconnected, this, &UiBridge::connectedChanged);
        connect(&m_socket, &QLocalSocket::readyRead, this, [this]() {
            while (m_socket.canReadLine()) {
                const QByteArray line = m_socket.readLine().trimmed();
                if (line.isEmpty()) {
                    continue;
                }
                QJsonParseError error{};
                const QJsonDocument doc = QJsonDocument::fromJson(line, &error);
                if (error.error != QJsonParseError::NoError || !doc.isObject()) {
                    continue;
                }
                const QJsonObject obj = doc.object();
                if (obj.contains("sector")) {
                    emit selectionReceived(obj.value("sector").toInt());
                }
            }
        });
    }

    Q_INVOKABLE void connectEngine() {
        if (m_socket.state() == QLocalSocket::ConnectedState) {
            return;
        }
        m_socket.connectToServer(socketPath());
    }

    Q_INVOKABLE void sendTouchDown(double x, double y) { sendJson("touch_down", x, y); }
    Q_INVOKABLE void sendTouchMove(double x, double y) { sendJson("touch_move", x, y); }
    Q_INVOKABLE void sendTouchUp(double x, double y) { sendJson("touch_up", x, y); }
    Q_INVOKABLE void sendAction(const QString &action) {
        QJsonObject obj;
        obj.insert("type", "action");
        obj.insert("action", action);
        sendObject(obj);
    }

    Q_INVOKABLE bool isConnected() const {
        return m_socket.state() == QLocalSocket::ConnectedState;
    }

    bool connected() const {
        return m_socket.state() == QLocalSocket::ConnectedState;
    }

signals:
    void connectedChanged();
    void selectionReceived(int sector);

private:
    void sendJson(const QString &type, double x, double y) {
        QJsonObject obj;
        obj.insert("type", type);
        obj.insert("x", x);
        obj.insert("y", y);
        sendObject(obj);
    }

    void sendObject(const QJsonObject &obj) {
        if (m_socket.state() != QLocalSocket::ConnectedState) {
            return;
        }
        const QJsonDocument doc(obj);
        m_socket.write(doc.toJson(QJsonDocument::Compact));
        m_socket.write("\n");
    }

    static QString socketPath() {
        const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
        if (!runtimeDir.isEmpty()) {
            return runtimeDir + "/radialkb.sock";
        }
        return QString("/tmp/radialkb-%1.sock").arg(getuid());
    }

    QLocalSocket m_socket;
};

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    UiBridge bridge;
    engine.rootContext()->setContextProperty("uiBridge", &bridge);
    engine.rootContext()->setContextProperty("qmlDir", QStringLiteral(RADIALKB_QML_DIR));

    const QUrl url = QUrl::fromLocalFile(QStringLiteral(RADIALKB_QML_DIR) + "/MainOverlay.qml");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}

#include "main.moc"
