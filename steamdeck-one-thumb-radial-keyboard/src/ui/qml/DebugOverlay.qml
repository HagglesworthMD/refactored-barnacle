import QtQuick 2.15

Rectangle {
    id: root
    property bool connected: false
    width: 160
    height: 32
    color: "#121315"
    radius: 6
    opacity: 0.7

    Text {
        anchors.centerIn: parent
        text: root.connected ? "Engine: connected" : "Engine: disconnected"
        color: root.connected ? "#8ce99a" : "#ffa94d"
        font.pixelSize: 12
    }
}
