import QtQuick 2.15

Item {
    id: root
    property int sectorCount: 8
    property int selectedSector: -1
    property int engineSelectedSector: -1
    property color baseColor: "#1e1f22"
    property color highlightColor: "#4aa3ff"
    property int activeSector: (uiBridge.connected && engineSelectedSector >= 0) ? engineSelectedSector : selectedSector

    signal selectionChanged(int sector)

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
        if (angle < 0) {
            angle += 2 * Math.PI
        }
        angle += Math.PI / 2
        if (angle >= 2 * Math.PI) {
            angle -= 2 * Math.PI
        }
        var index = Math.floor(angle / (2 * Math.PI / root.sectorCount))
        if (index !== root.selectedSector) {
            root.selectedSector = index
            root.selectionChanged(index)
            canvas.requestPaint()
        }
    }

    onSelectedSectorChanged: canvas.requestPaint()
    onActiveSectorChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()

    Connections {
        target: uiBridge
        function onSelectionReceived(sector) {
            root.engineSelectedSector = sector
        }
        function onConnectedChanged() {
            if (!uiBridge.connected) {
                root.engineSelectedSector = -1
            }
        }
    }
}
