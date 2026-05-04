import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects
import "components" // Ensures QML can find your DrumSettingsPane.qml

Item {
    id: root
    signal back()
    readonly property var drumZones: [
        { name: "Crash", note: 54, left: 0.02, top: 0.09, right: 0.25, bottom: 0.34, cymbal: true },
        { name: "High-Tom", note: 48, left: 0.31, top: 0.14, right: 0.47, bottom: 0.36, cymbal: false },
        { name: "Low-Tom", note: 45, left: 0.53, top: 0.14, right: 0.69, bottom: 0.36, cymbal: false },
        { name: "Ride", note: 60, left: 0.75, top: 0.09, right: 0.98, bottom: 0.34, cymbal: true },
        { name: "Closed Hi-Hat", note: 42, left: 0.02, top: 0.56, right: 0.28, bottom: 0.71, cymbal: true },
        { name: "Open Hi-Hat", note: 46, left: 0.02, top: 0.36, right: 0.28, bottom: 0.52, cymbal: true },
        { name: "Snare", note: 38, left: 0.29, top: 0.49, right: 0.48, bottom: 0.68, cymbal: false },
        { name: "Floor Tom", note: 43, left: 0.77, top: 0.49, right: 0.98, bottom: 0.71, cymbal: false },
        { name: "Kick", note: 36, left: 0.31, top: 0.75, right: 0.69, bottom: 0.96, cymbal: false }
    ]
    readonly property real circularZoneScale: 1.2

    function zoneContains(zone, x, y) {
        const centerX = (zone.left + zone.right) / 2
        const centerY = (zone.top + zone.bottom) / 2
        const rawRadiusX = (zone.right - zone.left) / 2
        const rawRadiusY = (zone.bottom - zone.top) / 2
        const radius = zone.name === "Kick" ? 0 : Math.min(rawRadiusX, rawRadiusY)
        const radiusX = zone.name === "Kick" ? rawRadiusX : radius
        const radiusY = zone.name === "Kick" ? rawRadiusY : radius

        if (radiusX <= 0 || radiusY <= 0)
            return false

        const dx = (x - centerX) / radiusX
        const dy = (y - centerY) / radiusY
        return (dx * dx) + (dy * dy) <= 1.0
    }

    function isZoneActive(zone) {
        const leftActive = appEngine.leftHandVisible
            && appEngine.leftPinch
            && zoneContains(zone, appEngine.leftHandX, appEngine.leftHandY)
        const rightActive = appEngine.rightHandVisible
            && appEngine.rightPinch
            && zoneContains(zone, appEngine.rightHandX, appEngine.rightHandY)
        return leftActive || rightActive
    }

    function stageX(normalizedX) {
        return normalizedX * drumStage.width
    }

    function stageY(normalizedY) {
        return normalizedY * drumStage.height
    }

    FontLoader {
        id: figTreeVariable
        source: "../assets/fonts/Figtree-VariableFont_wght.ttf"
    }

    // Dark background
    Rectangle {
        anchors.fill: parent
        color: "#101218"
    }

    // --- Python Camera Feed ---
    Image {
        id: cameraFeed
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        
        fillMode: Image.PreserveAspectFit 

        source: "image://camera/feed"
        cache: false 
        
        // This property makes the scaling look much smoother/sharper
        smooth: true
        mipmap: true

        Timer {
            interval: 33
            running: true
            repeat: true
            onTriggered: {
                cameraFeed.source = "image://camera/feed?id=" + Math.random()
            }
        }
    }

    Item {
        id: drumStage
        z: 5

        readonly property real contentWidth: cameraFeed.paintedWidth > 0 ? cameraFeed.paintedWidth : cameraFeed.width
        readonly property real contentHeight: cameraFeed.paintedHeight > 0 ? cameraFeed.paintedHeight : cameraFeed.height

        x: cameraFeed.x + ((cameraFeed.width - contentWidth) / 2)
        y: cameraFeed.y + ((cameraFeed.height - contentHeight) / 2)
        width: contentWidth
        height: contentHeight

        Repeater {
            model: root.drumZones

            delegate: Item {
                id: drumZone
                required property var modelData

                readonly property bool active: root.isZoneActive(modelData)
                readonly property bool kick: modelData.name === "Kick"
                readonly property real boundingWidth: (modelData.right - modelData.left) * drumStage.width
                readonly property real boundingHeight: (modelData.bottom - modelData.top) * drumStage.height
                readonly property real circleSize: Math.min(boundingWidth, boundingHeight)
                readonly property real scaledCircleSize: Math.min(
                    Math.min(drumStage.width, drumStage.height),
                    circleSize * root.circularZoneScale
                )

                x: (modelData.left * drumStage.width) + (kick ? 0 : ((boundingWidth - scaledCircleSize) / 2))
                y: (modelData.top * drumStage.height) + (kick ? 0 : ((boundingHeight - scaledCircleSize) / 2))
                width: kick ? boundingWidth : scaledCircleSize
                height: kick ? boundingHeight : scaledCircleSize

                Rectangle {
                    anchors.fill: parent
                    radius: kick ? height / 2 : Math.min(width, height) / 2
                    color: active ? "#12d8ff22" : "#08101922"
                    border.color: active ? "#FFF06A" : "#E7E8EC"
                    border.width: active ? 3 : 1
                    antialiasing: true
                }

                Item {
                    id: plateMask
                    anchors.fill: parent
                    anchors.margins: active ? 3 : 1

                    Image {
                        id: plateImage
                        anchors.fill: parent
                        source: modelData.cymbal ? "qrc:/assets/drums/cymbal.png" : "qrc:/assets/drums/drum.png"
                        fillMode: Image.PreserveAspectCrop
                        smooth: true
                        mipmap: true
                        visible: false
                    }

                    Rectangle {
                        id: plateShape
                        anchors.fill: parent
                        radius: kick ? height / 2 : Math.min(width, height) / 2
                        color: "#FFFFFF"
                        visible: false
                    }

                    OpacityMask {
                        id: maskedPlate
                        anchors.fill: parent
                        source: plateImage
                        maskSource: plateShape
                        opacity: 0.5
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: kick ? height / 2 : Math.min(width, height) / 2
                    color: active ? "#10f5ff24" : "#00000000"
                    border.width: 0
                    antialiasing: true
                }

                Text {
                    anchors.centerIn: parent
                    text: modelData.name
                    font.family: figTreeVariable.name
                    font.pixelSize: Math.max(11, Math.min(parent.width, parent.height) * 0.1)
                    font.weight: Font.Medium
                    color: active ? "#FFFFFF" : "#E2E4E8"
                    style: Text.Outline
                    styleColor: "#66000000"
                    z: 2
                }
            }
        }

        Rectangle {
            visible: appEngine.leftHandVisible
            width: 18
            height: 18
            radius: 9
            color: appEngine.leftPinch ? "#22E2FF" : "#7AD7FF"
            border.color: "#FFFFFF"
            border.width: 2
            x: root.stageX(appEngine.leftHandX) - (width / 2)
            y: root.stageY(appEngine.leftHandY) - (height / 2)
            z: 10
        }

        Rectangle {
            visible: appEngine.rightHandVisible
            width: 18
            height: 18
            radius: 9
            color: appEngine.rightPinch ? "#22E2FF" : "#7AD7FF"
            border.color: "#FFFFFF"
            border.width: 2
            x: root.stageX(appEngine.rightHandX) - (width / 2)
            y: root.stageY(appEngine.rightHandY) - (height / 2)
            z: 10
        }
    }

    // --- Header ---
    Item {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        z: 10

        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0.063, 0.071, 0.094, 0.75)
        }

        // Back arrow
        MouseArea {
            id: backBtn
            width: 48
            height: 48
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            
            // This tells C++ to switch the state back to the Session Page!
            onClicked: root.back()

            Text {
                anchors.centerIn: parent
                text: "\u2190"
                font.pixelSize: 24
                color: backBtn.containsMouse ? "#E07A26" : "#949AA5"
                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }

        // --- NEW: Settings gear ---
        MouseArea {
            id: settingsBtn
            width: 40
            height: 40
            anchors.left: backBtn.right
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            
            // Toggles the pane open/closed!
            onClicked: drumSettings.open = !drumSettings.open

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: settingsBtn.containsMouse || drumSettings.open
                       ? Qt.rgba(1, 1, 1, 0.08)
                       : "transparent"
                Behavior on color { ColorAnimation { duration: 150 } }

                Image {
                    anchors.centerIn: parent
                    width: 22
                    height: 22
                    source: "qrc:/assets/icons/settings.svg"
                    sourceSize.width: 22
                    sourceSize.height: 22
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    opacity: settingsBtn.containsMouse || drumSettings.open ? 1.0 : 0.85
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }
            }
        }

        // Title
        Text {
            anchors.centerIn: parent
            text: "Drums"
            font.family: figTreeVariable.name
            font.pixelSize: 20
            font.weight: Font.Light
            font.letterSpacing: 2
            color: "#E07826"
        }
    }

    // --- NEW: Settings pane overlay ---
    MouseArea {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: drumSettings.open
        onClicked: drumSettings.open = false
        z: 20
    }

    // --- NEW: Drum Settings Pane ---
    DrumSettingsPane {
        id: drumSettings
        property bool open: false

        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: header.bottom
        anchors.topMargin: 12
        width: 300
        z: 30

        font.family: figTreeVariable.name
        
        onClose: open = false

        visible: open
        opacity: open ? 1.0 : 0.0
        scale: open ? 1.0 : 0.96
        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
        Behavior on scale   { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
    }
}
