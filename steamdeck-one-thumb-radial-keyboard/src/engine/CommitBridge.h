#pragma once

#include <QChar>
#include <QString>

namespace radialkb {

struct KeyAction {
    enum Type { None, Char, Space, Backspace, Enter };
    Type type{None};
    char ch{'\0'};

    static KeyAction makeChar(char value) {
        KeyAction action;
        action.type = Char;
        action.ch = value;
        return action;
    }

    static KeyAction make(Type value) {
        KeyAction action;
        action.type = value;
        return action;
    }
};

class CommitBridge {
public:
    void commitChar(QChar ch);
    void commitAction(const QString &action);
    void commitAction(const KeyAction &action);
};

}
