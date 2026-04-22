import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property real masterVolume:  1.0   // 0.0 – 1.0  (global output gain)
    property real volumeFloor:   0.00  // 0.0 – 1.0  (min volume when left hand is at bottom)
    property int  semitoneRange: 48    // total semitone span (12–96)
    property int  centerNote:    60    // MIDI note at range centre (60 = C4)
    property alias font: titleLabel.font

    signal close()

    implicitWidth: 300
    implicitHeight: contentCol.implicitHeight + 32

    radius: 10
    color: "#1A1D26"
    border.width: 1
    border.color: Qt.rgba(1, 1, 1, 0.08)

    // Drop shadow
    Rectangle {
        z: -1
        anchors.fill: parent
        anchors.leftMargin: -2; anchors.rightMargin: -2
        anchors.topMargin: -2;  anchors.bottomMargin: -6
        radius: parent.radius + 2
        color: Qt.rgba(0, 0, 0, 0.35)
    }

    ColumnLayout {
        id: contentCol
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        // ── Header ──────────────────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            implicitHeight: 20

            Text {
                id: titleLabel
                text: "Settings"
                color: "#EBEDF0"
                font.pixelSize: 15
                font.weight: Font.Medium
                font.letterSpacing: 1.5
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
            }

            MouseArea {
                id: closeBtn
                width: 20; height: 20
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.close()

                Text {
                    anchors.centerIn: parent
                    text: "\u00D7"
                    font.pixelSize: 18
                    color: closeBtn.containsMouse ? "#E07A26" : "#949AA5"
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
            }
        }

        // ── Master volume ────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Master volume"
                    color: "#949AA5"; font.pixelSize: 12; font.letterSpacing: 1
                    Layout.fillWidth: true
                }
                Text {
                    text: (root.masterVolume * 100).toFixed(0) + "%"
                    color: "#EBEDF0"; font.pixelSize: 12; font.weight: Font.Medium
                }
            }

            Slider {
                id: masterSlider
                Layout.fillWidth: true
                from: 0; to: 1; stepSize: 0.01
                value: root.masterVolume
                onValueChanged: root.masterVolume = value

                background: Rectangle {
                    x: masterSlider.leftPadding
                    y: masterSlider.topPadding + masterSlider.availableHeight / 2 - height / 2
                    width: masterSlider.availableWidth; height: 4; radius: 2
                    color: Qt.rgba(1, 1, 1, 0.08)
                    Rectangle {
                        width: masterSlider.visualPosition * parent.width
                        height: parent.height; color: "#E07A26"; radius: 2
                    }
                }
                handle: Rectangle {
                    x: masterSlider.leftPadding + masterSlider.visualPosition * (masterSlider.availableWidth - width)
                    y: masterSlider.topPadding + masterSlider.availableHeight / 2 - height / 2
                    width: 14; height: 14; radius: 7
                    color: masterSlider.pressed ? "#E07A26" : "#EBEDF0"
                    border.color: "#E07A26"; border.width: 1
                }
            }
        }

        // ── Volume floor ─────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Volume floor"
                    color: "#949AA5"; font.pixelSize: 12; font.letterSpacing: 1
                    Layout.fillWidth: true
                }
                Text {
                    text: (root.volumeFloor * 100).toFixed(0) + "%"
                    color: "#EBEDF0"; font.pixelSize: 12; font.weight: Font.Medium
                }
            }

            Slider {
                id: floorSlider
                Layout.fillWidth: true
                from: 0; to: 0.5; stepSize: 0.01
                value: root.volumeFloor
                onValueChanged: root.volumeFloor = value

                background: Rectangle {
                    x: floorSlider.leftPadding
                    y: floorSlider.topPadding + floorSlider.availableHeight / 2 - height / 2
                    width: floorSlider.availableWidth; height: 4; radius: 2
                    color: Qt.rgba(1, 1, 1, 0.08)
                    Rectangle {
                        width: floorSlider.visualPosition * parent.width
                        height: parent.height; color: "#E07A26"; radius: 2
                    }
                }
                handle: Rectangle {
                    x: floorSlider.leftPadding + floorSlider.visualPosition * (floorSlider.availableWidth - width)
                    y: floorSlider.topPadding + floorSlider.availableHeight / 2 - height / 2
                    width: 14; height: 14; radius: 7
                    color: floorSlider.pressed ? "#E07A26" : "#EBEDF0"
                    border.color: "#E07A26"; border.width: 1
                }
            }
        }

        // ── Semitone range ────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Semitone range"
                    color: "#949AA5"; font.pixelSize: 12; font.letterSpacing: 1
                    Layout.fillWidth: true
                }
                Text {
                    text: root.semitoneRange + " st  (" + (root.semitoneRange / 12).toFixed(1) + " oct)"
                    color: "#EBEDF0"; font.pixelSize: 12; font.weight: Font.Medium
                }
            }

            Slider {
                id: semitoneSlider
                Layout.fillWidth: true
                from: 12; to: 96; stepSize: 12
                value: root.semitoneRange
                onValueChanged: root.semitoneRange = value

                background: Rectangle {
                    x: semitoneSlider.leftPadding
                    y: semitoneSlider.topPadding + semitoneSlider.availableHeight / 2 - height / 2
                    width: semitoneSlider.availableWidth; height: 4; radius: 2
                    color: Qt.rgba(1, 1, 1, 0.08)
                    Rectangle {
                        width: semitoneSlider.visualPosition * parent.width
                        height: parent.height; color: "#E07A26"; radius: 2
                    }
                }
                handle: Rectangle {
                    x: semitoneSlider.leftPadding + semitoneSlider.visualPosition * (semitoneSlider.availableWidth - width)
                    y: semitoneSlider.topPadding + semitoneSlider.availableHeight / 2 - height / 2
                    width: 14; height: 14; radius: 7
                    color: semitoneSlider.pressed ? "#E07A26" : "#EBEDF0"
                    border.color: "#E07A26"; border.width: 1
                }
            }
        }

        // ── Center note ───────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Center note"
                    color: "#949AA5"; font.pixelSize: 12; font.letterSpacing: 1
                    Layout.fillWidth: true
                }
                Text {
                    text: {
                        const names = ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"]
                        const oct = Math.floor(root.centerNote / 12) - 1
                        return names[root.centerNote % 12] + oct
                    }
                    color: "#EBEDF0"; font.pixelSize: 12; font.weight: Font.Medium
                }
            }

            Slider {
                id: centerNoteSlider
                Layout.fillWidth: true
                from: 24; to: 96; stepSize: 1
                value: root.centerNote
                onValueChanged: root.centerNote = value

                background: Rectangle {
                    x: centerNoteSlider.leftPadding
                    y: centerNoteSlider.topPadding + centerNoteSlider.availableHeight / 2 - height / 2
                    width: centerNoteSlider.availableWidth; height: 4; radius: 2
                    color: Qt.rgba(1, 1, 1, 0.08)
                    Rectangle {
                        width: centerNoteSlider.visualPosition * parent.width
                        height: parent.height; color: "#E07A26"; radius: 2
                    }
                }
                handle: Rectangle {
                    x: centerNoteSlider.leftPadding + centerNoteSlider.visualPosition * (centerNoteSlider.availableWidth - width)
                    y: centerNoteSlider.topPadding + centerNoteSlider.availableHeight / 2 - height / 2
                    width: 14; height: 14; radius: 7
                    color: centerNoteSlider.pressed ? "#E07A26" : "#EBEDF0"
                    border.color: "#E07A26"; border.width: 1
                }
            }
        }
    }
}
