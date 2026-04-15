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

        // Title logo (animated left-to-right wipe)
        Row {
            id: logoRow
            anchors.horizontalCenter: parent.horizontalCenter
            height: 77
            spacing: 0

            // "Air" container
            Item {
                id: airContainer
                width: 159.6
                height: 77

                Item {
                    id: wipeClipper
                    width: 0
                    height: 110
                    y: -17.7
                    clip: true

                    Image {
                        id: airImg
                        source: "qrc:/assets/icons/Air.svg"
                        width: 159.6
                        height: 109.4
                        fillMode: Image.Stretch
                        smooth: true
                        mipmap: true
                        sourceSize: Qt.size(159.6 * 2, 109.4 * 2)
                    }
                }
                
                // "Written in" animation
                SequentialAnimation {
                    running: true
                    PauseAnimation { duration: 400 }
                    ParallelAnimation {
                        NumberAnimation {
                            target: wipeClipper
                            property: "width"
                            from: 0
                            to: 159.6
                            duration: 1500
                            easing.type: Easing.InOutSine
                        }
                        NumberAnimation {
                            target: wipeClipper
                            property: "opacity"
                            from: 0.0
                            to: 1.0
                            duration: 1000
                            easing.type: Easing.OutQuad
                        }
                    }
                }
            }

            // "Chestra" container
            Item {
                id: chestraContainer
                width: 273.3
                height: 77

                Image {
                    id: chestraImg
                    source: "qrc:/assets/icons/Chestra.svg"
                    width: 273.3
                    height: 77
                    fillMode: Image.Stretch
                    smooth: true
                    mipmap: true
                    opacity: 0
                    sourceSize: Qt.size(273.3 * 2, 77 * 2)

                    SequentialAnimation on opacity {
                        PauseAnimation { duration: 400 + 1500 }
                        NumberAnimation {
                            from: 0.0
                            to: 1.0
                            duration: 1000
                            easing.type: Easing.OutQuad
                        }
                    }
                }
            }
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
