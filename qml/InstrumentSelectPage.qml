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

        // MIDI Selector
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12

            Text {
                text: "MIDI Output"
                font.pixelSize: 13
                font.weight: Font.Medium
                font.letterSpacing: 1
                color: "#949AA5"
                anchors.verticalCenter: parent.verticalCenter
            }

            TypeSelector {
                id: midiSelector
                width: 160
                model: ["None", "IAC Driver Bus 1", "Virtual MIDI Cable"]
                anchors.verticalCenter: parent.verticalCenter
                onCurrentTextChanged: {
                    if (currentText !== "None") {
                        appEngine.setMidiEnabled(true)
                    } else {
                        appEngine.setMidiEnabled(false)
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
