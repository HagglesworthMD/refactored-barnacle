#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>

class UiBridge : public QObject {
    Q_OBJECT
public:
    explicit UiBridge(QObject *parent = nullptr)
        : QObject(parent) {
        connect(&m_socket, &QLocalSocket::connected, this, &UiBridge::connectedChanged);
        connect(&m_socket, &QLocalSocket::disconnected, this, &UiBridge::connectedChanged);
    }

    Q_INVOKABLE void connectEngine() {
        if (m_socket.state() == QLocalSocket::ConnectedState) {
            return;
        }
        m_socket.connectToServer("/tmp/radialkb.sock");
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

signals:
    void connectedChanged();

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
