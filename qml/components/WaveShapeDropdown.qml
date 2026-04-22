import QtQuick
import QtQuick.Controls

// Custom dropdown for selecting a waveform. Shows the current wave's icon
// next to its label, and opens a popup with all three options.
Item {
    id: root

    // Two-way selection. Callers can bind to `value`.
    property string value: "sine"
    property alias font: label.font
    signal changed(string value)

    readonly property var options: [
        { key: "sine",     label: "Sine" },
        { key: "square",   label: "Square" },
        { key: "triangle", label: "Triangle" }
    ]

    implicitWidth: 160
    implicitHeight: 36

    function _labelFor(key) {
        for (let i = 0; i < options.length; ++i) {
            if (options[i].key === key) return options[i].label
        }
        return key
    }

    Rectangle {
        id: trigger
        anchors.fill: parent
        radius: height / 2
        color: triggerHover.containsMouse || popup.opened
               ? Qt.rgba(1, 1, 1, 0.08)
               : Qt.rgba(1, 1, 1, 0.04)
        border.width: 1
        border.color: popup.opened
                      ? "#E07A26"
                      : (triggerHover.containsMouse ? Qt.rgba(1, 1, 1, 0.20)
                                                    : Qt.rgba(1, 1, 1, 0.12))

        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 12
            spacing: 10

            WaveIcon {
                shape: root.value
                strokeColor: "#EBEDF0"
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                id: label
                text: root._labelFor(root.value)
                color: "#EBEDF0"
                font.pixelSize: 13
                font.weight: Font.Medium
                font.letterSpacing: 1
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // Chevron
        Text {
            text: "\u25BE"
            color: "#949AA5"
            font.pixelSize: 12
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
        }

        MouseArea {
            id: triggerHover
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: popup.opened ? popup.close() : popup.open()
        }
    }

    Popup {
        id: popup
        y: trigger.height + 6
        x: 0
        width: trigger.width
        padding: 6

        background: Rectangle {
            color: "#1A1D26"
            border.color: Qt.rgba(1, 1, 1, 0.10)
            border.width: 1
            radius: 8
        }

        contentItem: Column {
            spacing: 2

            Repeater {
                model: root.options

                delegate: Rectangle {
                    width: popup.width - popup.padding * 2
                    height: 34
                    radius: 6
                    color: optionHover.containsMouse
                           ? Qt.rgba(0.878, 0.478, 0.149, 0.18)
                           : "transparent"

                    Behavior on color { ColorAnimation { duration: 120 } }

                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        spacing: 10

                        WaveIcon {
                            shape: modelData.key
                            strokeColor: optionHover.containsMouse || root.value === modelData.key
                                         ? "#E07A26"
                                         : "#EBEDF0"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: modelData.label
                            color: optionHover.containsMouse || root.value === modelData.key
                                   ? "#E07A26"
                                   : "#EBEDF0"
                            font.pixelSize: 13
                            font.weight: Font.Medium
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        id: optionHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (root.value !== modelData.key) {
                                root.value = modelData.key
                                root.changed(modelData.key)
                            }
                            popup.close()
                        }
                    }
                }
            }
        }
    }
}
