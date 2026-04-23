import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property alias font: titleLabel.font
    signal close()

    implicitWidth: 300
    implicitHeight: contentCol.implicitHeight + 32

    radius: 10
    color: "#1A1D26"
    border.width: 1
    border.color: Qt.rgba(1, 1, 1, 0.08)

    Rectangle {
        z: -1
        anchors.fill: parent
        anchors.margins: -2
        anchors.bottomMargin: -6
        radius: parent.radius + 2
        color: Qt.rgba(0, 0, 0, 0.35)
    }

    ColumnLayout {
        id: contentCol
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        // Header
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
                width: 20
                height: 20
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

        // --- THE FIX: REAL MIDI DROPDOWN AND TOGGLE ---
        Text {
            text: "MIDI OUTPUT"
            font.family: titleLabel.font.family
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.letterSpacing: 1.5
            color: "#E07826"
            Layout.topMargin: 10
        }

        SettingsToggle {
            label: "Enable MIDI"
            Layout.fillWidth: true
            // Read initial state from C++, write back changes
            checked: typeof appEngine !== "undefined" && appEngine.viewState ? appEngine.viewState.midiEnabled : false
            onCheckedChanged: {
                if (typeof appEngine !== "undefined") {
                    appEngine.setMidiEnabled(checked)
                }
            }
        }

        ComboBox {
            id: midiDeviceSelect
            Layout.fillWidth: true
            height: 40
            // Pull live list of MIDI devices from JUCE backend!
            model: typeof appEngine !== "undefined" ? appEngine.midiDeviceNames : ["None"]
            
            onActivated: function(index) {
                if (typeof appEngine !== "undefined") {
                    appEngine.selectMidiDevice(currentText)
                }
            }

            background: Rectangle {
                color: Qt.rgba(1, 1, 1, 0.05)
                radius: 6
                border.color: Qt.rgba(1, 1, 1, 0.1)
            }
            contentItem: Text {
                text: midiDeviceSelect.currentText
                color: "#EBEDF0"
                font.pixelSize: 14
                verticalAlignment: Text.AlignVCenter
                leftPadding: 12
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Qt.rgba(1, 1, 1, 0.06) }

        // --- THE FIX: REAL THEREMIN RANGE SETTINGS ---
        Text {
            text: "PITCH RANGE"
            font.family: titleLabel.font.family
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.letterSpacing: 1.5
            color: "#E07826"
        }

        SettingsSlider {
            label: "Center Note"
            unit: " (MIDI)"
            from: 24
            to: 96
            stepSize: 1
            Layout.fillWidth: true
            value: typeof appEngine !== "undefined" && appEngine.viewState ? appEngine.viewState.thereminCenterNote : 60
            onValueChanged: {
                if (typeof appEngine !== "undefined") {
                    appEngine.setThereminCenterNote(Math.round(value))
                }
            }
        }

        SettingsSlider {
            label: "Semitone Range"
            unit: "st"
            from: 12
            to: 96
            stepSize: 1
            Layout.fillWidth: true
            value: typeof appEngine !== "undefined" && appEngine.viewState ? appEngine.viewState.thereminSemitoneRange : 24
            onValueChanged: {
                if (typeof appEngine !== "undefined") {
                    appEngine.setThereminSemitoneRange(Math.round(value))
                }
            }
        }
    }
}