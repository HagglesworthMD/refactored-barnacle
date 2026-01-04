import QtQuick 2.15

Item {
    id: root
    property int sectorCount: 8
    property int selectedSector: -1
    property int selectedLetter: -1
    property int engineSelectedSector: -1
    property int engineSelectedLetter: -1
    property color baseColor: "#1e1f22"
    property color highlightColor: "#4aa3ff"
    property color letterColor: "#cfd2d8"
    property color letterHighlight: "#f8f9fa"
    property int activeSector: (uiBridge.connected && engineSelectedSector >= 0) ? engineSelectedSector : selectedSector
    property int activeLetter: (uiBridge.connected && engineSelectedLetter >= 0) ? engineSelectedLetter : selectedLetter
    property real deadzoneRadius: 0.12
    property real innerRadius: 0.28
    property real innerHysteresis: 0.03
    property bool localTrackingLetter: false
    property bool hoverTracking: false
    property real lastHoverX: width / 2
    property real lastHoverY: height / 2
    property var sectorKeys: [
        ["E", "T", "A", "O"],
        ["I", "N", "S", "H"],
        ["R", "D", "L", "U"],
        ["C", "M", "F", "W"],
        ["G", "P", "B", "Y"],
        ["V", "K", "J", "X"],
        ["Q", "Z", ".", ",", "?"],
        ["␠", "⌫", "↵"]
    ]

    // ─────────────────────────────────────────────────────────────
    // Animation state (UI-only, does not affect engine/commit logic)
    // ─────────────────────────────────────────────────────────────
    property real sectorPulseScale: 1.0       // Animates on sector change
    property real sectorPulseOpacity: 0.0     // Pulse glow opacity
    property real letterPopScale: 1.0         // Animates on letter change
    property real commitGlowIntensity: 0.0    // Commit flash effect
    property real magneticOffsetX: 0.0        // Magnetic pull toward pointer
    property real magneticOffsetY: 0.0
    property real smoothPointerX: width / 2   // Smoothed pointer position
    property real smoothPointerY: height / 2

    Behavior on smoothPointerX { SmoothedAnimation { velocity: 1200; duration: 80 } }
    Behavior on smoothPointerY { SmoothedAnimation { velocity: 1200; duration: 80 } }

    signal selectionChanged(int sector, int letter)

    Canvas {
        id: canvas
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            var centerX = width / 2
            var centerY = height / 2
            var radius = Math.min(width, height) * 0.45
            var sectorSize = 2 * Math.PI / root.sectorCount

            // Draw sectors with pulse animation
            for (var i = 0; i < root.sectorCount; i++) {
                var isActiveSector = (i === root.activeSector)
                var sectorRadius = radius * (isActiveSector ? root.sectorPulseScale : 1.0)
                var startAngle = (i * sectorSize) - Math.PI / 2
                var endAngle = startAngle + sectorSize

                ctx.save()
                ctx.translate(centerX, centerY)
                if (isActiveSector && root.sectorPulseScale !== 1.0) {
                    ctx.scale(root.sectorPulseScale, root.sectorPulseScale)
                }
                ctx.beginPath()
                ctx.moveTo(0, 0)
                ctx.arc(0, 0, radius, startAngle, endAngle)
                ctx.closePath()
                ctx.fillStyle = isActiveSector ? root.highlightColor : root.baseColor
                ctx.fill()
                ctx.strokeStyle = "#2a2c30"
                ctx.lineWidth = 2
                ctx.stroke()

                // Pulse glow overlay
                if (isActiveSector && root.sectorPulseOpacity > 0.01) {
                    ctx.beginPath()
                    ctx.moveTo(0, 0)
                    ctx.arc(0, 0, radius, startAngle, endAngle)
                    ctx.closePath()
                    ctx.fillStyle = "rgba(74, 163, 255," + root.sectorPulseOpacity + ")"
                    ctx.fill()
                }
                ctx.restore()
            }

            // Draw letters with pop/magnetic animation
            ctx.textAlign = "center"
            ctx.textBaseline = "middle"
            ctx.font = "16px sans-serif"
            for (var sectorIndex = 0; sectorIndex < root.sectorCount; sectorIndex++) {
                var keys = root.sectorKeys[sectorIndex] || []
                var start = (sectorIndex * sectorSize) - Math.PI / 2
                var perKey = sectorSize / Math.max(keys.length, 1)
                for (var keyIndex = 0; keyIndex < keys.length; keyIndex++) {
                    var keyAngle = start + perKey * (keyIndex + 0.5)
                    var textRadius = radius * 0.68
                    var textX = centerX + Math.cos(keyAngle) * textRadius
                    var textY = centerY + Math.sin(keyAngle) * textRadius
                    var isActive = (sectorIndex === root.activeSector) && (keyIndex === root.activeLetter)

                    // Apply magnetic offset to active letter
                    if (isActive) {
                        textX += root.magneticOffsetX
                        textY += root.magneticOffsetY
                    }

                    var scale = isActive ? root.letterPopScale : 1.0

                    // Letter background circle
                    if (isActive) {
                        ctx.save()
                        ctx.translate(textX, textY)
                        if (scale !== 1.0) {
                            ctx.scale(scale, scale)
                        }
                        ctx.beginPath()
                        ctx.fillStyle = "rgba(255,255,255,0.18)"
                        ctx.arc(0, 0, 16, 0, 2 * Math.PI)
                        ctx.fill()

                        // Commit glow overlay
                        if (root.commitGlowIntensity > 0.01) {
                            ctx.beginPath()
                            ctx.fillStyle = "rgba(74, 163, 255," + root.commitGlowIntensity + ")"
                            ctx.arc(0, 0, 20, 0, 2 * Math.PI)
                            ctx.fill()
                        }
                        ctx.restore()
                    }

                    // Letter text
                    ctx.save()
                    ctx.translate(textX, textY)
                    if (scale !== 1.0) {
                        ctx.scale(scale, scale)
                    }
                    ctx.fillStyle = isActive ? root.letterHighlight : root.letterColor
                    ctx.fillText(keys[keyIndex], 0, 0)
                    ctx.restore()
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onExited: {
            // Some Qt versions do not provide mouse.buttons on onExited; use MouseArea state.
            if (pressed) {
                return
            }
            if (root.hoverTracking) {
                root.commitSelection()
                uiBridge.sendTouchUp(root.normalizedX(root.lastHoverX), root.normalizedY(root.lastHoverY))
                root.hoverTracking = false
            }
            root.updateSelection(width / 2, height / 2)
        }
        onPressed: (mouse) => {
            if (mouse.button !== Qt.LeftButton) {
                return
            }
            root.updateSelection(mouse.x, mouse.y)
            uiBridge.sendTouchDown(root.normalizedX(mouse.x), root.normalizedY(mouse.y))
        }
        onPositionChanged: (mouse) => {
            root.smoothPointerX = mouse.x
            root.smoothPointerY = mouse.y
            if (mouse.buttons & Qt.LeftButton) {
                root.lastHoverX = mouse.x
                root.lastHoverY = mouse.y
                root.updateSelection(mouse.x, mouse.y)
                uiBridge.sendTouchMove(root.normalizedX(mouse.x), root.normalizedY(mouse.y))
                return
            }
            if (!root.hoverTracking) {
                root.hoverTracking = true
                uiBridge.sendTouchDown(root.normalizedX(mouse.x), root.normalizedY(mouse.y))
            }
            root.lastHoverX = mouse.x
            root.lastHoverY = mouse.y
            root.updateSelection(mouse.x, mouse.y)
            uiBridge.sendTouchMove(root.normalizedX(mouse.x), root.normalizedY(mouse.y))
        }
        onReleased: (mouse) => {
            if (mouse.button !== Qt.LeftButton) {
                return
            }
            root.commitSelection()
            uiBridge.sendTouchUp(root.normalizedX(mouse.x), root.normalizedY(mouse.y))
        }
        onClicked: (mouse) => {
            if (mouse.button === Qt.RightButton) {
                uiBridge.sendAction("enter")
            }
        }
    }

    function normalizedX(x) {
        var value = x / width
        return Math.max(0, Math.min(1, value))
    }

    function normalizedY(y) {
        var value = y / height
        return Math.max(0, Math.min(1, value))
    }

    function updateSelection(x, y) {
        if (uiBridge.connected) {
            return
        }
        var dx = x - width / 2
        var dy = y - height / 2
        var angle = Math.atan2(dy, dx)
        var radius = Math.sqrt(dx * dx + dy * dy) / (Math.min(width, height) * 0.5)
        if (angle < 0) {
            angle += 2 * Math.PI
        }
        angle += Math.PI / 2
        if (angle >= 2 * Math.PI) {
            angle -= 2 * Math.PI
        }
        if (radius < root.deadzoneRadius) {
            if (root.selectedSector !== -1 || root.selectedLetter !== -1) {
                root.selectedSector = -1
                root.selectedLetter = -1
                root.localTrackingLetter = false
                root.selectionChanged(-1, -1)
                canvas.requestPaint()
            }
            return
        }
        var index = Math.floor(angle / (2 * Math.PI / root.sectorCount))
        if (index !== root.selectedSector) {
            root.selectedSector = index
            root.selectionChanged(index, root.selectedLetter)
            canvas.requestPaint()
        }

        if (!root.localTrackingLetter && radius >= root.innerRadius) {
            root.localTrackingLetter = true
        } else if (root.localTrackingLetter && radius < (root.innerRadius - root.innerHysteresis)) {
            root.localTrackingLetter = false
        }

        if (root.localTrackingLetter) {
            var keys = root.sectorKeys[index] || []
            if (keys.length > 0) {
                var localAngle = angle - (index * (2 * Math.PI / root.sectorCount))
                if (localAngle < 0) {
                    localAngle += 2 * Math.PI
                }
                var keyIndex = Math.floor(localAngle / ((2 * Math.PI / root.sectorCount) / keys.length))
                if (keyIndex !== root.selectedLetter) {
                    root.selectedLetter = keyIndex
                    root.selectionChanged(index, keyIndex)
                }
            }
        } else if (root.selectedLetter !== -1) {
            root.selectedLetter = -1
            root.selectionChanged(index, -1)
            canvas.requestPaint()
        }
    }

    function commitSelection() {
        if (!uiBridge.connected || root.activeSector < 0) {
            return false
        }
        var keys = root.sectorKeys[root.activeSector] || []
        var keyIndex = root.activeLetter
        if (keyIndex < 0) {
            keyIndex = 0
        }
        if (keyIndex >= keys.length) {
            return false
        }
        var label = keys[keyIndex]
        if (!label || label.length === 0) {
            return false
        }
        var charToSend = label
        if (label === "␠") {
            charToSend = " "
        } else if (label === "⌫") {
            charToSend = "\b"
        } else if (label === "↵") {
            charToSend = "\n"
        }
        console.log("[UI] Commit:", label)
        commitGlowAnim.restart()
        uiBridge.sendChar(charToSend)
        if (label === "␠") {
            return true
        }
        return true
    }

    onSelectedSectorChanged: canvas.requestPaint()
    onSelectedLetterChanged: canvas.requestPaint()
    onActiveSectorChanged: {
        if (activeSector >= 0) {
            console.log("[UI] Sector changed:", activeSector)
            sectorPulseAnim.restart()
        }
        canvas.requestPaint()
    }
    onActiveLetterChanged: {
        if (activeLetter >= 0) {
            console.log("[UI] Letter changed:", activeLetter)
            letterPopAnim.restart()
        }
        canvas.requestPaint()
    }
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()
    onSectorPulseScaleChanged: canvas.requestPaint()
    onLetterPopScaleChanged: canvas.requestPaint()
    onCommitGlowIntensityChanged: canvas.requestPaint()
    onMagneticOffsetXChanged: canvas.requestPaint()
    onMagneticOffsetYChanged: canvas.requestPaint()

    Connections {
        target: uiBridge
        function onSelectionReceived(sector, letter, stage, clearSelection) {
            if (clearSelection || sector === -1) {
                root.engineSelectedSector = -1
                root.engineSelectedLetter = -1
                return
            }
            root.engineSelectedSector = sector
            root.engineSelectedLetter = letter
        }
        function onConnectedChanged() {
            if (!uiBridge.connected) {
                root.engineSelectedSector = -1
                root.engineSelectedLetter = -1
            } else {
                root.localTrackingLetter = false
            }
        }
    }

    // ─────────────────────────────────────────────────────────────
    // UI-only animations (do not affect engine/commit logic)
    // ─────────────────────────────────────────────────────────────
    SequentialAnimation {
        id: sectorPulseAnim
        ParallelAnimation {
            NumberAnimation { target: root; property: "sectorPulseScale"; from: 1.0; to: 1.08; duration: 80; easing.type: Easing.OutQuad }
            NumberAnimation { target: root; property: "sectorPulseOpacity"; from: 0.0; to: 0.25; duration: 80; easing.type: Easing.OutQuad }
        }
        ParallelAnimation {
            NumberAnimation { target: root; property: "sectorPulseScale"; to: 1.0; duration: 120; easing.type: Easing.InOutQuad }
            NumberAnimation { target: root; property: "sectorPulseOpacity"; to: 0.0; duration: 120; easing.type: Easing.InQuad }
        }
    }

    SequentialAnimation {
        id: letterPopAnim
        NumberAnimation { target: root; property: "letterPopScale"; from: 1.0; to: 1.15; duration: 60; easing.type: Easing.OutBack; easing.overshoot: 2.0 }
        NumberAnimation { target: root; property: "letterPopScale"; to: 1.0; duration: 100; easing.type: Easing.InOutQuad }
    }

    SequentialAnimation {
        id: commitGlowAnim
        NumberAnimation { target: root; property: "commitGlowIntensity"; from: 0.0; to: 0.5; duration: 50; easing.type: Easing.OutQuad }
        NumberAnimation { target: root; property: "commitGlowIntensity"; to: 0.0; duration: 200; easing.type: Easing.InQuad }
    }

    // Magnetic offset calculation (smooth spring-like pull toward pointer)
    Timer {
        interval: 16
        running: root.activeLetter >= 0
        repeat: true
        onTriggered: {
            if (root.activeSector < 0 || root.activeLetter < 0) {
                root.magneticOffsetX = 0
                root.magneticOffsetY = 0
                return
            }
            var centerX = root.width / 2
            var centerY = root.height / 2
            var radius = Math.min(root.width, root.height) * 0.45
            var sectorSize = 2 * Math.PI / root.sectorCount
            var start = (root.activeSector * sectorSize) - Math.PI / 2
            var keys = root.sectorKeys[root.activeSector] || []
            var perKey = sectorSize / Math.max(keys.length, 1)
            var keyAngle = start + perKey * (root.activeLetter + 0.5)
            var textRadius = radius * 0.68
            var keyX = centerX + Math.cos(keyAngle) * textRadius
            var keyY = centerY + Math.sin(keyAngle) * textRadius

            var toPointerX = root.smoothPointerX - keyX
            var toPointerY = root.smoothPointerY - keyY
            var dist = Math.sqrt(toPointerX * toPointerX + toPointerY * toPointerY)
            var maxPull = 4.0
            var pullFactor = Math.min(1.0, dist / (radius * 0.3))

            var targetOffsetX = (dist > 0.1) ? (toPointerX / dist) * maxPull * pullFactor : 0
            var targetOffsetY = (dist > 0.1) ? (toPointerY / dist) * maxPull * pullFactor : 0

            var spring = 0.3
            root.magneticOffsetX += (targetOffsetX - root.magneticOffsetX) * spring
            root.magneticOffsetY += (targetOffsetY - root.magneticOffsetY) * spring
        }
    }
}
