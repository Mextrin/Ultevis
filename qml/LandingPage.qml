import QtQuick
import QtQuick.Controls
import "components"

Item {
    id: landing
    signal proceed()

    FontLoader {
        id: figTreeVariable
        source: "../assets/fonts/Figtree-VariableFont_wght.ttf"
    }

    // Background
    Rectangle {
        anchors.fill: parent
        color: "#101218"
    }

    // Sine wave layer
    SineWaveCanvas {
        anchors.fill: parent
    }

    // Floating musical notes
    Repeater {
        model: 15
        FloatingNote {}
    }

    // Small drifting particles for depth
    Repeater {
        model: 25
        Rectangle {
            property real startX: Math.random() * (landing.width)
            property real startY: Math.random() * (landing.height)
            property real driftDuration: 10000 + Math.random() * 10000

            width: 2 + Math.random() * 3
            height: width
            radius: width / 2
            color: Qt.rgba(0.878, 0.478, 0.149, 0.03 + Math.random() * 0.05)
            x: startX
            y: startY

            SequentialAnimation on y {
                loops: Animation.Infinite
                NumberAnimation { to: -20; duration: driftDuration; easing.type: Easing.Linear }
                NumberAnimation { to: landing.height + 20; duration: 0 }
            }

            NumberAnimation on x {
                from: startX - 30
                to: startX + 30
                duration: driftDuration * 0.7
                loops: Animation.Infinite
                easing.type: Easing.InOutSine
            }
        }
    }

    // Centered content
    Column {
        anchors.centerIn: parent
        spacing: 0

        // Title
        Text {
            text: "Airchestra"
            font.family: figTreeVariable.name
            font.pixelSize: 72
            font.weight: Font.Bold
            font.letterSpacing: 2
            color: "#E07A26"
            anchors.horizontalCenter: parent.horizontalCenter

            // Subtle glow via drop shadow effect
            layer.enabled: true
            layer.effect: null
        }

        Item { width: 1; height: 12 }

        // Accent line
        Rectangle {
            width: 100
            height: 2
            color: "#e8e6e5ff"
            anchors.horizontalCenter: parent.horizontalCenter
            opacity: 0.8
        }

        Item { width: 1; height: 16 }

        // Subtitle
        Text {
            text: "Gesture-Controlled Instrumentation"
            font.pixelSize: 22
            font.weight: Font.Normal
            font.letterSpacing: 3
            color: "#EBEDF0"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Item { width: 1; height: 60 }

        // Click to continue hint
        Text {
            id: hintText
            text: "Click anywhere to continue"
            font.pixelSize: 14
            font.weight: Font.Normal
            font.letterSpacing: 1
            color: "#949AA5"
            anchors.horizontalCenter: parent.horizontalCenter

            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { to: 0.3; duration: 1500; easing.type: Easing.InOutQuad }
                NumberAnimation { to: 0.9; duration: 1500; easing.type: Easing.InOutQuad }
            }
        }
    }

    // Full-screen click area
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: landing.proceed()
    }
}
