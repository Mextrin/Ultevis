import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Floating settings card. Mock-only values — not yet wired to C++.
Rectangle {
    id: root

    // Bindable values exposed to the parent page.
    property real volumeFloor: 0.2          // 0.0 – 1.0
    property real freqMin: 220              // Hz
    property real freqMax: 880              // Hz
    property alias font: titleLabel.font

    signal close()

    implicitWidth: 300
    implicitHeight: contentCol.implicitHeight + 32

    radius: 10
    color: "#1A1D26"
    border.width: 1
    border.color: Qt.rgba(1, 1, 1, 0.08)

    // Soft drop shadow approximation (an offset dimmer layer behind the card).
    Rectangle {
        z: -1
        anchors.fill: parent
        anchors.leftMargin: -2
        anchors.rightMargin: -2
        anchors.topMargin: -2
        anchors.bottomMargin: -6
        radius: parent.radius + 2
        color: Qt.rgba(0, 0, 0, 0.35)
    }

    ColumnLayout {
        id: contentCol
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        // Header with title + close
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

        // Volume floor
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Volume floor"
                    color: "#949AA5"
                    font.pixelSize: 12
                    font.letterSpacing: 1
                    Layout.fillWidth: true
                }
                Text {
                    text: (root.volumeFloor * 100).toFixed(0) + "%"
                    color: "#EBEDF0"
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
            }

            Slider {
                id: volumeSlider
                Layout.fillWidth: true
                from: 0
                to: 1
                stepSize: 0.01
                value: root.volumeFloor
                onValueChanged: root.volumeFloor = value

                background: Rectangle {
                    x: volumeSlider.leftPadding
                    y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                    width: volumeSlider.availableWidth
                    height: 4
                    radius: 2
                    color: Qt.rgba(1, 1, 1, 0.08)

                    Rectangle {
                        width: volumeSlider.visualPosition * parent.width
                        height: parent.height
                        color: "#E07A26"
                        radius: 2
                    }
                }

                handle: Rectangle {
                    x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                    y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                    width: 14
                    height: 14
                    radius: 7
                    color: volumeSlider.pressed ? "#E07A26" : "#EBEDF0"
                    border.color: "#E07A26"
                    border.width: 1
                }
            }
        }

        // Frequency range
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "Frequency range (Hz)"
                color: "#949AA5"
                font.pixelSize: 12
                font.letterSpacing: 1
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                // Min
                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    radius: 6
                    color: Qt.rgba(1, 1, 1, 0.04)
                    border.width: 1
                    border.color: minField.activeFocus ? "#E07A26" : Qt.rgba(1, 1, 1, 0.10)
                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    Text {
                        text: "Min"
                        color: "#949AA5"
                        font.pixelSize: 11
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    TextInput {
                        id: minField
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 50
                        horizontalAlignment: TextInput.AlignRight
                        text: Math.round(root.freqMin).toString()
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 20; top: 4000 }
                        color: "#EBEDF0"
                        selectByMouse: true
                        font.pixelSize: 13
                        onEditingFinished: {
                            const v = parseInt(text)
                            if (!isNaN(v)) {
                                root.freqMin = Math.min(v, root.freqMax - 1)
                                text = Math.round(root.freqMin).toString()
                            }
                        }
                    }
                }

                // Max
                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    radius: 6
                    color: Qt.rgba(1, 1, 1, 0.04)
                    border.width: 1
                    border.color: maxField.activeFocus ? "#E07A26" : Qt.rgba(1, 1, 1, 0.10)
                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    Text {
                        text: "Max"
                        color: "#949AA5"
                        font.pixelSize: 11
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    TextInput {
                        id: maxField
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 50
                        horizontalAlignment: TextInput.AlignRight
                        text: Math.round(root.freqMax).toString()
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 20; top: 4000 }
                        color: "#EBEDF0"
                        selectByMouse: true
                        font.pixelSize: 13
                        onEditingFinished: {
                            const v = parseInt(text)
                            if (!isNaN(v)) {
                                root.freqMax = Math.max(v, root.freqMin + 1)
                                text = Math.round(root.freqMax).toString()
                            }
                        }
                    }
                }
            }
        }

        // Hint
        Text {
            text: "Mock settings \u2014 values are UI-only for now."
            color: "#5B6170"
            font.pixelSize: 10
            font.italic: true
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
    }
}
