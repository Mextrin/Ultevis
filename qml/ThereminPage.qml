import QtQuick
import QtQuick.Controls
import "components"

Item {
    id: root
    signal back()

    // UI state
    property string waveShape: "sine"
    property real volumeFloor: 0.2
    property real freqMin: 220
    property real freqMax: 880

    FontLoader {
        id: figTreeVariable
        source: "../assets/fonts/Figtree-VariableFont_wght.ttf"
    }

    // Dark background
    Rectangle {
        anchors.fill: parent
        color: "#101218"
    }

    // --- NEW: Python Shared Memory Camera Feed -------------------------------
    Image {
        id: cameraFeed
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        
        // --- CHANGE THIS LINE ---
        fillMode: Image.PreserveAspectFit 
        // ------------------------

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

    // Fallback overlay (shows while waiting for permissions or Python to boot)
    Rectangle {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: "#101218"
        // Hide the overlay once permission is granted (Python takes over the feed)
        visible: appEngine.cameraPermissionStatus !== "granted"
        z: 5

        Column {
            anchors.centerIn: parent
            spacing: 10
            width: Math.min(parent.width - 80, 520)

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "\u25A1"
                font.pixelSize: 44
                color: "#949AA5"
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: appEngine.cameraPermissionStatus === "denied" ? "Camera access denied" : "Waiting for camera\u2026"
                font.family: figTreeVariable.name
                font.pixelSize: 18
                font.weight: Font.Medium
                color: "#EBEDF0"
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: appEngine.cameraPermissionStatus === "denied"
                      ? "Open System Settings \u2192 Privacy & Security \u2192 Camera and enable Airchestra, then restart the app."
                      : "Grant camera access in the system prompt to see the live feed."
                font.family: figTreeVariable.name
                font.pixelSize: 13
                color: "#949AA5"
                lineHeight: 1.3
            }
        }
    }

    Component.onCompleted: {
        // Ask for permissions right away if we don't have them yet
        if (appEngine.cameraPermissionStatus === "undetermined") {
            appEngine.requestCameraPermission()
        }
    }

    // --- Dotted guide line (30% from the left, full main-area height) --------
    Canvas {
        id: guideLine
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.save()
            ctx.strokeStyle = Qt.rgba(0.922, 0.929, 0.941, 0.8) // #EBEDF0 @ 80%
            ctx.lineWidth = 6
            ctx.setLineDash([6, 8])
            const x = Math.round(width * 0.30) + 0.5
            ctx.beginPath()
            ctx.moveTo(x, 0)
            ctx.lineTo(x, height)
            ctx.stroke()
            ctx.restore()
        }
    }

    // --- Header --------------------------------------------------------------
    Item {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        z: 10

        // Subtle dark backdrop over the camera feed so controls stay legible.
        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0.063, 0.071, 0.094, 0.75)
        }

        // Back arrow (far left)
        MouseArea {
            id: backBtn
            width: 48
            height: 48
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.back()

            Text {
                anchors.centerIn: parent
                text: "\u2190"
                font.pixelSize: 24
                color: backBtn.containsMouse ? "#E07A26" : "#949AA5"
                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }

        // Settings gear
        MouseArea {
            id: settingsBtn
            width: 40
            height: 40
            anchors.left: backBtn.right
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: settingsPane.open = !settingsPane.open

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: settingsBtn.containsMouse || settingsPane.open
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
                    opacity: settingsBtn.containsMouse || settingsPane.open ? 1.0 : 0.85
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }
            }
        }

        // Title (center)
        Text {
            anchors.centerIn: parent
            text: "Theremin"
            font.family: figTreeVariable.name
            font.pixelSize: 20
            font.weight: Font.Light
            font.letterSpacing: 2
            color: "#E07826"
        }

        // Wave dropdown (far right)
        WaveShapeDropdown {
            id: waveDropdown
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            value: root.waveShape
            font.family: figTreeVariable.name
            onChanged: function(v) { root.waveShape = v }
        }
    }

    // --- Settings pane overlay ----------------------------------------------
    MouseArea {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: settingsPane.open
        onClicked: settingsPane.open = false
        z: 20
    }

    SettingsPane {
        id: settingsPane
        property bool open: false

        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: header.bottom
        anchors.topMargin: 12
        width: 300
        z: 30

        font.family: figTreeVariable.name
        volumeFloor: root.volumeFloor
        freqMin: root.freqMin
        freqMax: root.freqMax

        onVolumeFloorChanged: root.volumeFloor = volumeFloor
        onFreqMinChanged: root.freqMin = freqMin
        onFreqMaxChanged: root.freqMax = freqMax
        onClose: open = false

        visible: open
        opacity: open ? 1.0 : 0.0
        scale: open ? 1.0 : 0.96
        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
        Behavior on scale   { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
    }
}