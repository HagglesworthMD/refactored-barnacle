import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    id: overlay
    width: 520
    height: 520
    color: "transparent"
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
    visibility: Window.Windowed
    visible: true

    Component.onCompleted: {
        uiBridge.connectEngine()
    }

    RadialKeyboard {
        id: keyboard
        anchors.centerIn: parent
        width: 480
        height: 480
    }

    CandidateBar {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: keyboard.bottom
        anchors.topMargin: 6
        width: 320
        height: 32
    }

    DebugOverlay {
        anchors.left: parent.left
        anchors.top: parent.top
        connected: uiBridge.connected
    }
}
