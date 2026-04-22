import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Item {
    id: root
    signal back()
    signal instrumentSelected(string name)

    FontLoader {
        id: figTreeVariable
        source: "../assets/fonts/Figtree-VariableFont_wght.ttf"
    }

    Rectangle {
        anchors.fill: parent
        color: "#101218"
    }

    // Header bar
    Item {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60

        // Back arrow
        MouseArea {
            id: backBtn
            width: 48
            height: 48
            anchors.left: parent.left
            anchors.leftMargin: 20
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

        // Title
        Text {
            anchors.centerIn: parent
            text: "Select Instrument"
            font.pixelSize: 20
            font.weight: Font.Light
            font.letterSpacing: 2
            color: "#EBEDF0"
        }

        // MIDI toggle
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            Text {
                text: "MIDI"
                font.pixelSize: 13
                font.weight: Font.Medium
                font.letterSpacing: 1
                color: midiToggle.checked ? "#E07A26" : "#949AA5"
                anchors.verticalCenter: parent.verticalCenter
                Behavior on color { ColorAnimation { duration: 200 } }
            }

            // Custom toggle switch
            Rectangle {
                id: midiToggle
                property bool checked: false
                width: 44
                height: 24
                radius: 12
                color: checked ? Qt.rgba(0.878, 0.478, 0.149, 0.3) : Qt.rgba(1, 1, 1, 0.08)
                border.width: 1
                border.color: checked ? "#E07A26" : Qt.rgba(1, 1, 1, 0.12)
                anchors.verticalCenter: parent.verticalCenter

                Behavior on color { ColorAnimation { duration: 200 } }
                Behavior on border.color { ColorAnimation { duration: 200 } }

                Rectangle {
                    id: knob
                    width: 18
                    height: 18
                    radius: 9
                    color: midiToggle.checked ? "#E07A26" : "#949AA5"
                    anchors.verticalCenter: parent.verticalCenter
                    x: midiToggle.checked ? midiToggle.width - width - 3 : 3

                    Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }
                    Behavior on color { ColorAnimation { duration: 200 } }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        midiToggle.checked = !midiToggle.checked
                        appEngine.setMidiEnabled(midiToggle.checked)
                    }
                }
            }
        }
    }

    // Separator line
    Rectangle {
        id: sep
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 40
        anchors.rightMargin: 40
        height: 1
        color: Qt.rgba(1, 1, 1, 0.06)
    }

    // Quadrant grid
    Item {
        id: gridContainer
        anchors.top: sep.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 30
        anchors.topMargin: 20

        GridLayout {
            anchors.fill: parent
            rows: 2
            columns: 2
            rowSpacing: 16
            columnSpacing: 16

            InstrumentCard {
                instrumentName: "Keyboard"
                iconSource: "qrc:/assets/icons/piano.svg"
                Layout.fillWidth: true
                Layout.fillHeight: true
                font.family: figTreeVariable.name
                onClicked: root.instrumentSelected("keyboard")
            }

            InstrumentCard {
                instrumentName: "Theremin"
                iconSource: "qrc:/assets/icons/theremin.svg"
                Layout.fillWidth: true
                Layout.fillHeight: true
                font.family: figTreeVariable.name
                onClicked: root.instrumentSelected("theremin")
            }

            InstrumentCard {
                instrumentName: "Drums"
                iconSource: "qrc:/assets/icons/drums.svg"
                Layout.fillWidth: true
                Layout.fillHeight: true
                font.family: figTreeVariable.name
                onClicked: root.instrumentSelected("drums")
            }

            InstrumentCard {
                instrumentName: "Guitar"
                iconSource: "qrc:/assets/icons/guitar.svg"
                Layout.fillWidth: true
                Layout.fillHeight: true
                font.family: figTreeVariable.name
                onClicked: root.instrumentSelected("guitar")
            }
        }
    }
}
