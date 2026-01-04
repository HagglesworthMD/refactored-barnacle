import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtCore

Window {
    id: overlay
    // Steam Deck left-trackpad ergonomic placement
    readonly property real pxPerMm: 5.0   // Steam Deck ~= 5 px/mm
    readonly property real kbDiameterMm: 74
    readonly property real kbDiameterPx: kbDiameterMm * pxPerMm
    readonly property int leftPadOffsetX: 110
    readonly property int leftPadOffsetY: Screen.height - height - 120
    readonly property int candidateBarHeight: 32
    readonly property int candidateBarMargin: 6

    width: kbDiameterPx
    height: kbDiameterPx + candidateBarHeight + candidateBarMargin
    color: "transparent"
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
    visibility: Window.Windowed
    visible: true
    opacity: uiSettings.kbOpacity
    x: uiSettings.overlayX
    y: uiSettings.overlayY

    // Persisted UI preferences
    Settings {
        id: uiSettings
        property real kbOpacity: 0.85
        property int overlayX: leftPadOffsetX
        property int overlayY: leftPadOffsetY
    }

    Component.onCompleted: {
        console.log("[UI] Position loaded from settings: x=" + uiSettings.overlayX + " y=" + uiSettings.overlayY)
        uiBridge.connectEngine()
    }

    RadialKeyboard {
        id: keyboard
        anchors.centerIn: parent
        width: kbDiameterPx
        height: kbDiameterPx
    }

    CandidateBar {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: keyboard.bottom
        anchors.topMargin: candidateBarMargin
        width: 320
        height: candidateBarHeight
    }

    DebugOverlay {
        anchors.left: parent.left
        anchors.top: parent.top
        connected: uiBridge.connected
    }

    // ─────────────────────────────────────────────────────────────
    // Transparency control (does not steal focus)
    // ─────────────────────────────────────────────────────────────
    Popup {
        id: opacityPopup
        parent: overlay.contentItem
        modal: false
        focus: false
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

        x: Math.max(8, overlay.width - width - 8)
        y: 8

        background: Rectangle {
            radius: 10
            color: "#1e1e1e"
            opacity: 0.92
            border.width: 1
            border.color: "#3a3a3a"
        }

        contentItem: Column {
            spacing: 8
            padding: 10

            Text {
                text: "Transparency"
                color: "white"
                font.pixelSize: 14
            }

            Row {
                spacing: 10
                Slider {
                    id: opacitySlider
                    from: 0.30
                    to: 1.00
                    stepSize: 0.05
                    value: uiSettings.kbOpacity
                    onMoved: {
                        uiSettings.kbOpacity = value
                        console.log("[UI] KB opacity =", uiSettings.kbOpacity.toFixed(2))
                    }
                    width: 180
                }

                Text {
                    text: Math.round(uiSettings.kbOpacity * 100) + "%"
                    color: "white"
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    // Tiny opacity button on-keyboard (top-right), thumb-friendly, no focus grab
    Rectangle {
        id: opacityButton
        width: 26
        height: 26
        radius: 13
        color: "#000000"
        opacity: 0.35
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 6
        anchors.rightMargin: 6

        Text {
            anchors.centerIn: parent
            text: "OP"
            color: "white"
            font.pixelSize: 10
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onEntered: opacityButton.opacity = 0.55
            onExited: opacityButton.opacity = 0.35
            onClicked: {
                if (opacityPopup.opened) opacityPopup.close()
                else opacityPopup.open()
            }
        }
    }

    // ─────────────────────────────────────────────────────────────
    // Drag handle (top-left), requires long-press to activate
    // ─────────────────────────────────────────────────────────────
    property bool dragActive: false
    property real dragStartMouseX: 0
    property real dragStartMouseY: 0
    property real dragStartWindowX: 0
    property real dragStartWindowY: 0

    function clampPosition(newX, newY) {
        var margin = 20
        var clampedX = Math.max(margin, Math.min(Screen.width - width - margin, newX))
        var clampedY = Math.max(margin, Math.min(Screen.height - height - margin, newY))
        return Qt.point(clampedX, clampedY)
    }

    function savePosition() {
        uiSettings.overlayX = overlay.x
        uiSettings.overlayY = overlay.y
        console.log("[UI] Position saved: x=" + overlay.x + " y=" + overlay.y)
    }

    Rectangle {
        id: dragHandle
        width: 32
        height: 32
        radius: 16
        color: "#000000"
        opacity: dragHandleMouseArea.pressed ? 0.6 : (dragHandleMouseArea.containsMouse ? 0.45 : 0.30)
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 6
        anchors.leftMargin: 6

        Text {
            anchors.centerIn: parent
            text: "⋮⋮"
            color: "white"
            font.pixelSize: 12
            rotation: 90
        }

        MouseArea {
            id: dragHandleMouseArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            property bool longPressActivated: false

            Timer {
                id: longPressTimer
                interval: 250
                repeat: false
                onTriggered: {
                    dragHandleMouseArea.longPressActivated = true
                    overlay.dragActive = true
                    console.log("[UI] Drag activated (long-press detected)")
                }
            }

            onPressed: (mouse) => {
                longPressActivated = false
                overlay.dragStartMouseX = mouse.x
                overlay.dragStartMouseY = mouse.y
                overlay.dragStartWindowX = overlay.x
                overlay.dragStartWindowY = overlay.y
                longPressTimer.start()
            }

            onReleased: {
                longPressTimer.stop()
                if (overlay.dragActive) {
                    overlay.dragActive = false
                    overlay.savePosition()
                    console.log("[UI] Drag ended")
                }
                longPressActivated = false
            }

            onPositionChanged: (mouse) => {
                if (overlay.dragActive && longPressActivated) {
                    var deltaX = mouse.x - overlay.dragStartMouseX
                    var deltaY = mouse.y - overlay.dragStartMouseY
                    var newX = overlay.dragStartWindowX + deltaX
                    var newY = overlay.dragStartWindowY + deltaY
                    var clamped = overlay.clampPosition(newX, newY)
                    overlay.x = clamped.x
                    overlay.y = clamped.y
                }
            }
        }
    }
}
