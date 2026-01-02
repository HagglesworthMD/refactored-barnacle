#include "Logging.h"

#include <QDateTime>
#include <QTextStream>

namespace radialkb {

void logWithTag(const QString &tag, const QString &message) {
    QTextStream out(stdout);
    out << QDateTime::currentDateTime().toString(Qt::ISODate)
        << " " << tag << " " << message << "\n";
    out.flush();
}

}
