#pragma once

#include <QChar>
#include <QString>

namespace radialkb {

class CommitBridge {
public:
    void commitChar(QChar ch);
    void commitAction(const QString &action);
};

}
