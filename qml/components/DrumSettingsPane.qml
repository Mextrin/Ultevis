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

        // --- HEADER ---
        Item {
            Layout.fillWidth: true
            implicitHeight: 20

            Text {
                id: titleLabel
                text: "Drum Settings"
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

        // --- MIDI OUTPUT SECTION ---
        Text {
            text: "MIDI OUTPUT"
            font.family: titleLabel.font.family
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.letterSpacing: 1.5
            color: "#E07826"
            Layout.topMargin: 10
        }

        ComboBox {
            id: midiDeviceSelect
            Layout.fillWidth: true
            height: 40
            model: typeof appEngine !== "undefined" ? appEngine.midiDeviceNames : ["None"]
            currentIndex: 0

            Component.onCompleted: {
                if (typeof appEngine !== "undefined") {
                    let idx = find(appEngine.currentMidiDevice)
                    if (idx !== -1) {
                        currentIndex = idx
                    }
                }
            }

            Connections {
                target: typeof appEngine !== "undefined" ? appEngine : null
                function onCurrentMidiDeviceChanged() {
                    let idx = midiDeviceSelect.find(appEngine.currentMidiDevice)
                    if (idx !== -1 && midiDeviceSelect.currentIndex !== idx) {
                        midiDeviceSelect.currentIndex = idx
                    }
                }
            }
            
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

        SettingsSlider {
            label: "Master Volume"
            unit: "%"
            from: 0
            to: 100
            stepSize: 1
            Layout.fillWidth: true
            
            // Read state from C++
            value: typeof appEngine !== "undefined" && appEngine.viewState ? (appEngine.viewState.masterVolume * 100) : 100
            
            onValueChanged: {
                if (typeof appEngine !== "undefined") {
                    appEngine.setMasterVolume(value / 100.0)
                }
            }
        }

        // Separator Line
        Rectangle { Layout.fillWidth: true; height: 1; color: Qt.rgba(1, 1, 1, 0.06) }
    }
}