import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property real masterVolume: 1.0          // 0.0 – 1.0  (global output gain)
    property real volumeFloor:  0.2          // 0.0 – 1.0  (theremin minimum volume)
    property real freqMin: 220               // Hz
    property real freqMax: 880               // Hz
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
                from: 0; to: 1; stepSize: 0.01
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

        // ── Frequency range ───────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "Frequency range (Hz)"
                color: "#949AA5"; font.pixelSize: 12; font.letterSpacing: 1
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Rectangle {
                    Layout.fillWidth: true; height: 32; radius: 6
                    color: Qt.rgba(1, 1, 1, 0.04)
                    border.width: 1
                    border.color: minField.activeFocus ? "#E07A26" : Qt.rgba(1, 1, 1, 0.10)
                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    Text {
                        text: "Min"; color: "#949AA5"; font.pixelSize: 11
                        anchors.left: parent.left; anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    TextInput {
                        id: minField
                        anchors.right: parent.right; anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 50
                        horizontalAlignment: TextInput.AlignRight
                        text: Math.round(root.freqMin).toString()
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 20; top: 4000 }
                        color: "#EBEDF0"; selectByMouse: true; font.pixelSize: 13
                        onEditingFinished: {
                            const v = parseInt(text)
                            if (!isNaN(v)) {
                                root.freqMin = Math.min(v, root.freqMax - 1)
                                text = Math.round(root.freqMin).toString()
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true; height: 32; radius: 6
                    color: Qt.rgba(1, 1, 1, 0.04)
                    border.width: 1
                    border.color: maxField.activeFocus ? "#E07A26" : Qt.rgba(1, 1, 1, 0.10)
                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    Text {
                        text: "Max"; color: "#949AA5"; font.pixelSize: 11
                        anchors.left: parent.left; anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    TextInput {
                        id: maxField
                        anchors.right: parent.right; anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 50
                        horizontalAlignment: TextInput.AlignRight
                        text: Math.round(root.freqMax).toString()
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 20; top: 4000 }
                        color: "#EBEDF0"; selectByMouse: true; font.pixelSize: 13
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
    }
}
