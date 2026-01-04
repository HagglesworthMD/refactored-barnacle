#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusConnection>
#include <QDBusError>
#include <QStandardPaths>
#include <QWindow>
#include <QPointer>
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
                const bool clearSelection = obj.value("clearSelection").toBool(false);
                if (obj.contains("sector") || clearSelection) {
                    int sector = obj.value("sector").toInt(-1);
                    int letter = obj.value("letter").toInt(-1);
                    const QString stage = obj.value("stage").toString();
                    if (clearSelection) {
                        sector = -1;
                        letter = -1;
                    }
                    emit selectionReceived(sector, letter, stage, clearSelection);
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
    Q_INVOKABLE void sendChar(const QString &ch) {
        const QString trimmed = ch.left(1).toLower();
        if (trimmed.isEmpty()) {
            return;
        }
        QJsonObject obj;
        obj.insert("type", "commit_char");
        obj.insert("char", trimmed);
        sendObject(obj);
    }
    void sendUiShow() { sendType("ui_show"); }
    void sendUiHide() { sendType("ui_hide"); }
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
    void selectionReceived(int sector, int letter, const QString &stage, bool clearSelection);

private:
    void sendType(const QString &type) {
        QJsonObject obj;
        obj.insert("type", type);
        sendObject(obj);
    }

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

class OverlayController : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.radialkb.Overlay")
public:
    OverlayController(QWindow *window, UiBridge *bridge, QObject *parent = nullptr)
        : QObject(parent), m_window(window), m_bridge(bridge) {}

public slots:
    void Show() { setVisible(true); }
    void Hide() { setVisible(false); }
    void Toggle() {
        if (!m_window) {
            return;
        }
        setVisible(!m_window->isVisible());
    }
    bool Status() const { return m_window && m_window->isVisible(); }

private:
    void setVisible(bool visible) {
        if (!m_window) {
            return;
        }
        m_window->setVisible(visible);
        if (m_bridge) {
            visible ? m_bridge->sendUiShow() : m_bridge->sendUiHide();
        }
    }

    QPointer<QWindow> m_window;
    UiBridge *m_bridge = nullptr;
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

    QWindow *rootWindow = nullptr;
    if (!engine.rootObjects().isEmpty()) {
        rootWindow = qobject_cast<QWindow *>(engine.rootObjects().first());
    }
    OverlayController controller(rootWindow, &bridge);
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.isConnected()) {
        if (!bus.registerService(QStringLiteral("org.radialkb.Overlay"))) {
            qWarning() << "Failed to register D-Bus service org.radialkb.Overlay:"
                       << bus.lastError().message();
        }
        if (!bus.registerObject(QStringLiteral("/org/radialkb/Overlay"), &controller,
                                QDBusConnection::ExportAllSlots)) {
            qWarning() << "Failed to register D-Bus object /org/radialkb/Overlay:"
                       << bus.lastError().message();
        }
    } else {
        qWarning() << "D-Bus session bus unavailable:" << bus.lastError().message();
    }

    return app.exec();
}

#include "main.moc"
