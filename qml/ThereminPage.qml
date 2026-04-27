import QtQuick
import QtQuick.Controls
import "components"

Item {
    id: root
    signal back()

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

    // --- Dotted guide line ---
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
            ctx.strokeStyle = Qt.rgba(0.922, 0.929, 0.941, 0.8) 
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

        // Title 
        Text {
            anchors.centerIn: parent
            text: "Theremin"
            font.family: figTreeVariable.name
            font.pixelSize: 20
            font.weight: Font.Light
            font.letterSpacing: 2
            color: "#E07826"
        }

        // Wave dropdown
        WaveShapeDropdown {
            id: waveDropdown
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            
            // Read initial state from C++
            value: typeof appEngine !== "undefined" && appEngine.viewState && appEngine.viewState.thereminWaveform !== undefined ? appEngine.viewState.thereminWaveform : "sine"
            font.family: figTreeVariable.name
            
            // Write changes to C++
            onChanged: function(v) { 
                if (typeof appEngine !== "undefined") {
                    appEngine.setThereminWaveform(v) 
                }
            }
        }
    }

    // --- Settings pane overlay ---
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
        
        // --- THE FIX: ALL freqMin/freqMax BINDINGS DELETED HERE ---

        onClose: open = false

        visible: open
        opacity: open ? 1.0 : 0.0
        scale: open ? 1.0 : 0.96
        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
        Behavior on scale   { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
    }
}