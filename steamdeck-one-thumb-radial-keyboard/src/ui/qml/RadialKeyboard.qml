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

            for (var i = 0; i < root.sectorCount; i++) {
                var startAngle = (i * sectorSize) - Math.PI / 2
                var endAngle = startAngle + sectorSize
                ctx.beginPath()
                ctx.moveTo(centerX, centerY)
                ctx.arc(centerX, centerY, radius, startAngle, endAngle)
                ctx.closePath()
                ctx.fillStyle = (i === root.activeSector) ? root.highlightColor : root.baseColor
                ctx.fill()
                ctx.strokeStyle = "#2a2c30"
                ctx.lineWidth = 2
                ctx.stroke()
            }

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
                    if (isActive) {
                        ctx.beginPath()
                        ctx.fillStyle = "rgba(255,255,255,0.18)"
                        ctx.arc(textX, textY, 16, 0, 2 * Math.PI)
                        ctx.fill()
                    }
                    ctx.fillStyle = isActive ? root.letterHighlight : root.letterColor
                    ctx.fillText(keys[keyIndex], textX, textY)
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onPressed: (mouse) => {
            if (mouse.button !== Qt.LeftButton) {
                return
            }
            root.updateSelection(mouse.x, mouse.y)
            uiBridge.sendTouchDown(root.normalizedX(mouse.x), root.normalizedY(mouse.y))
        }
        onPositionChanged: (mouse) => {
            if (!(mouse.buttons & Qt.LeftButton)) {
                return
            }
            root.updateSelection(mouse.x, mouse.y)
            uiBridge.sendTouchMove(root.normalizedX(mouse.x), root.normalizedY(mouse.y))
        }
        onReleased: (mouse) => {
            if (mouse.button !== Qt.LeftButton) {
                return
            }
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

    onSelectedSectorChanged: canvas.requestPaint()
    onSelectedLetterChanged: canvas.requestPaint()
    onActiveSectorChanged: canvas.requestPaint()
    onActiveLetterChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()

    Connections {
        target: uiBridge
        function onSelectionReceived(sector, letter, stage) {
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
}
