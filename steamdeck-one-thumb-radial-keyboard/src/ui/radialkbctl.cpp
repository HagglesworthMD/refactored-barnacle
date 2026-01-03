#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QTextStream>

namespace {

int printUsage(const QString &appName) {
    QTextStream err(stderr);
    err << "Usage: " << appName << " toggle|show|hide|status\n";
    return 2;
}

bool ensureInterfaceValid(const QDBusInterface &iface) {
    return iface.isValid() && QDBusConnection::sessionBus().isConnected();
}

int printStatus(QDBusInterface &iface) {
    QDBusReply<bool> reply = iface.call(QStringLiteral("Status"));
    if (!reply.isValid()) {
        QTextStream err(stderr);
        err << "radialkbctl: D-Bus Status call failed: " << reply.error().message() << "\n";
        return 1;
    }
    QTextStream out(stdout);
    out << "visible=" << (reply.value() ? "true" : "false") << "\n";
    return 0;
}

int callAndReport(QDBusInterface &iface, const QString &method) {
    QDBusMessage reply = iface.call(method);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        QTextStream err(stderr);
        err << "radialkbctl: D-Bus " << method << " failed: " << reply.errorMessage() << "\n";
        return 1;
    }
    return printStatus(iface);
}

} // namespace

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.size() != 2) {
        return printUsage(args.value(0, QStringLiteral("radialkbctl")));
    }

    QDBusInterface iface(QStringLiteral("org.radialkb.Overlay"),
                         QStringLiteral("/org/radialkb/Overlay"),
                         QStringLiteral("org.radialkb.Overlay"),
                         QDBusConnection::sessionBus());
    if (!ensureInterfaceValid(iface)) {
        QTextStream err(stderr);
        err << "radialkbctl: overlay service is not running.\n";
        return 1;
    }

    const QString command = args.at(1).toLower();
    if (command == "status") {
        return printStatus(iface);
    }
    if (command == "toggle") {
        return callAndReport(iface, QStringLiteral("Toggle"));
    }
    if (command == "show") {
        return callAndReport(iface, QStringLiteral("Show"));
    }
    if (command == "hide") {
        return callAndReport(iface, QStringLiteral("Hide"));
    }

    return printUsage(args.value(0, QStringLiteral("radialkbctl")));
}
