import QtQuick 2.15

// TouchKeyboard.qml
// iOS-style touchscreen QWERTY keyboard for Steam Deck.
// Physical sizing based on mm via screenMetrics.
// Phase 1: tap-to-commit, no swipe typing yet.
Item {
    id: root

    property bool active: true

    // Physical dimensions (mm)
    readonly property real keyboardHeightMm: 48.0
    readonly property real keyHeightMm: 8.5
    readonly property real keySpacingMm: 0.8
    readonly property real bottomMarginMm: 7.0

    // Computed pixel dimensions via screenMetrics
    readonly property real keyboardHeight: screenMetrics.mmToPx(keyboardHeightMm)
    readonly property real keyHeight: screenMetrics.mmToPx(keyHeightMm)
    readonly property real keySpacing: screenMetrics.mmToPx(keySpacingMm)
    readonly property real sidePadding: 14

    // Currently pressed key for visual feedback
    property string pressedKey: ""

    // Styling (match existing RadialKeyboard colors)
    readonly property color baseColor: "#2a2c30"
    readonly property color highlightColor: "#4aa3ff"
    readonly property color textColor: "#cfd2d8"
    readonly property color highlightTextColor: "#ffffff"
    readonly property color keyBorderColor: "#3a3a3a"

    height: keyboardHeight

    Rectangle {
        id: background
        anchors.fill: parent
        color: "#1e1f22"
        opacity: 0.94
        radius: 12
    }

    Column {
        id: keyboardLayout
        anchors.fill: parent
        anchors.leftMargin: sidePadding
        anchors.rightMargin: sidePadding
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        spacing: keySpacing

        // Row 1: QWERTYUIOP
        KeyRow {
            keys: ["q", "w", "e", "r", "t", "y", "u", "i", "o", "p"]
            rowInset: 0
        }

        // Row 2: ASDFGHJKL
        KeyRow {
            keys: ["a", "s", "d", "f", "g", "h", "j", "k", "l"]
            rowInset: screenMetrics.mmToPx(4)
        }

        // Row 3: Shift + ZXCVBNM + Backspace
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: keySpacing

            // Shift key (visual only in Phase 1)
            Rectangle {
                width: root.keyHeight * 1.3
                height: root.keyHeight
                radius: 8
                color: root.baseColor
                border.color: root.keyBorderColor
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "\u21e7"  // Shift symbol
                    color: root.textColor
                    font.pixelSize: 18
                }
            }

            Repeater {
                model: ["z", "x", "c", "v", "b", "n", "m"]

                Rectangle {
                    property string keyChar: modelData
                    width: (keyboardLayout.width - root.keyHeight * 2.6 - 9 * keySpacing - sidePadding * 2) / 7
                    height: root.keyHeight
                    radius: 8
                    color: root.pressedKey === keyChar ? root.highlightColor : root.baseColor
                    border.color: root.keyBorderColor
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: keyChar.toUpperCase()
                        color: root.pressedKey === keyChar ? root.highlightTextColor : root.textColor
                        font.pixelSize: 18
                        font.weight: Font.Medium
                    }

                    TapHandler {
                        onPressedChanged: {
                            if (pressed) root.pressedKey = keyChar
                        }
                        onTapped: {
                            if (!root.active) return
                            root.pressedKey = ""
                            root.commitKey(keyChar)
                        }
                        onCanceled: root.pressedKey = ""
                    }
                }
            }

            // Backspace key
            Rectangle {
                width: root.keyHeight * 1.3
                height: root.keyHeight
                radius: 8
                color: root.pressedKey === "backspace" ? root.highlightColor : root.baseColor
                border.color: root.keyBorderColor
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "\u232b"  // Backspace symbol
                    color: root.pressedKey === "backspace" ? root.highlightTextColor : root.textColor
                    font.pixelSize: 18
                }

                TapHandler {
                    onPressedChanged: {
                        if (pressed) root.pressedKey = "backspace"
                    }
                    onTapped: {
                        if (!root.active) return
                        root.pressedKey = ""
                        root.commitKey("backspace")
                    }
                    onCanceled: root.pressedKey = ""
                }
            }
        }

        // Row 4: 123 + Space + Return
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: keySpacing

            // 123 key (visual only in Phase 1)
            Rectangle {
                width: root.keyHeight * 1.5
                height: root.keyHeight
                radius: 8
                color: root.baseColor
                border.color: root.keyBorderColor
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "123"
                    color: root.textColor
                    font.pixelSize: 14
                }
            }

            // Space bar
            Rectangle {
                width: keyboardLayout.width - root.keyHeight * 4.5 - 4 * keySpacing - sidePadding * 2
                height: root.keyHeight
                radius: 8
                color: root.pressedKey === "space" ? root.highlightColor : "#3a3c40"
                border.color: root.keyBorderColor
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "space"
                    color: root.pressedKey === "space" ? root.highlightTextColor : root.textColor
                    font.pixelSize: 14
                }

                TapHandler {
                    onPressedChanged: {
                        if (pressed) root.pressedKey = "space"
                    }
                    onTapped: {
                        if (!root.active) return
                        root.pressedKey = ""
                        root.commitKey("space")
                    }
                    onCanceled: root.pressedKey = ""
                }
            }

            // Return key
            Rectangle {
                width: root.keyHeight * 1.5
                height: root.keyHeight
                radius: 8
                color: root.pressedKey === "enter" ? root.highlightColor : root.highlightColor.darker(1.3)
                border.color: root.keyBorderColor
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "return"
                    color: root.highlightTextColor
                    font.pixelSize: 14
                }

                TapHandler {
                    onPressedChanged: {
                        if (pressed) root.pressedKey = "enter"
                    }
                    onTapped: {
                        if (!root.active) return
                        root.pressedKey = ""
                        root.commitKey("enter")
                    }
                    onCanceled: root.pressedKey = ""
                }
            }
        }
    }

    // Commit key via UiBridge
    function commitKey(key) {
        if (key === "space") {
            uiBridge.sendAction("space")
        } else if (key === "backspace") {
            uiBridge.sendAction("backspace")
        } else if (key === "enter") {
            uiBridge.sendAction("enter")
        } else {
            uiBridge.sendChar(key)
        }
    }

    // Reusable key row component
    component KeyRow: Row {
        property var keys: []
        property real rowInset: 0

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.leftMargin: rowInset
        anchors.rightMargin: rowInset
        spacing: keySpacing

        Repeater {
            model: keys

            Rectangle {
                property string keyChar: modelData
                width: (keyboardLayout.width - (keys.length - 1) * keySpacing - rowInset * 2 - sidePadding * 2) / keys.length
                height: root.keyHeight
                radius: 8
                color: root.pressedKey === keyChar ? root.highlightColor : root.baseColor
                border.color: root.keyBorderColor
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: keyChar.toUpperCase()
                    color: root.pressedKey === keyChar ? root.highlightTextColor : root.textColor
                    font.pixelSize: 18
                    font.weight: Font.Medium
                }

                TapHandler {
                    onPressedChanged: {
                        if (pressed) root.pressedKey = keyChar
                    }
                    onTapped: {
                        if (!root.active) return
                        root.pressedKey = ""
                        root.commitKey(keyChar)
                    }
                    onCanceled: root.pressedKey = ""
                }
            }
        }
    }
}
