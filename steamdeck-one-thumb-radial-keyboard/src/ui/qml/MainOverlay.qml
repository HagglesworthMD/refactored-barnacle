import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import Qt.labs.settings 1.1

// INTENT: Overlay window must remain non-focus-stealing; show/hide should avoid flicker.
// INTENT: Future: draggable UI + pin toggle should not disrupt active input context.

Window {
    id: overlay

    // Mode detection: touch mode via RADIALKB_INPUT=touch
    readonly property bool touchMode: screenMetrics.touchMode

    // Radial mode: Steam Deck left-trackpad ergonomic placement
    readonly property real pxPerMm: 5.0   // Steam Deck ~= 5 px/mm
    readonly property real kbDiameterMm: 74
    readonly property real kbDiameterPx: kbDiameterMm * pxPerMm
    readonly property int leftPadOffsetX: 110
    readonly property int leftPadOffsetY: Screen.height - height - 120
    readonly property int candidateBarHeight: 32
    readonly property int candidateBarMargin: 6

    // Touch mode: full-width keyboard at bottom
    readonly property real touchKbHeight: screenMetrics.mmToPx(48)
    readonly property real touchKbMargin: screenMetrics.mmToPx(7)

    // Dynamic sizing based on mode
    width: touchMode ? Screen.width : kbDiameterPx
    height: touchMode ? (touchKbHeight + touchKbMargin + candidateBarHeight) : (kbDiameterPx + candidateBarHeight + candidateBarMargin)
    color: "transparent"
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
    visibility: Window.Windowed
    visible: true
    opacity: uiSettings.kbOpacity
    x: touchMode ? 0 : leftPadOffsetX
    y: touchMode ? (Screen.height - height) : leftPadOffsetY

    // Persisted UI preferences
    Settings {
        id: uiSettings
        property real kbOpacity: 0.85
    }

    Component.onCompleted: {
        uiBridge.connectEngine()
    }

    // Keyboard loader: RadialKeyboard (trackpad) or TouchKeyboard (touchscreen)
    Loader {
        id: keyboardLoader
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: candidateBar.top
        anchors.bottomMargin: candidateBarMargin

        sourceComponent: overlay.touchMode ? touchKeyboardComponent : radialKeyboardComponent
    }

    Component {
        id: radialKeyboardComponent
        RadialKeyboard {
            anchors.centerIn: parent
            width: kbDiameterPx
            height: kbDiameterPx
        }
    }

    Component {
        id: touchKeyboardComponent
        TouchKeyboard {
            anchors.fill: parent
            active: true
        }
    }

    CandidateBar {
        id: candidateBar
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: overlay.touchMode ? touchKbMargin : 0
        width: overlay.touchMode ? parent.width - 40 : 320
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
}
