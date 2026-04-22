import QtQuick
import QtQuick.Controls
import "components"

Item {
    id: root
    signal back()

    FontLoader {
        id: figTreeVariable
        source: "../assets/fonts/Figtree-VariableFont_wght.ttf"
    }

    Rectangle {
        anchors.fill: parent
        color: "#101218"
    }

    // Webcam placeholder
    Rectangle {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: "#0A0C10"

        Text {
            anchors.centerIn: parent
            text: "Camera Feed"
            font.pixelSize: 18
            font.weight: Font.Light
            font.letterSpacing: 2
            color: Qt.rgba(1, 1, 1, 0.15)
        }
    }

    // --- Header --------------------------------------------------------------
    Item {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        z: 10

        // Subtle dark backdrop over the camera feed so controls stay legible.
        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0.063, 0.071, 0.094, 0.75)
        }

        // Back arrow (far left)
        MouseArea {
            id: backBtn
            width: 48
            height: 48
            anchors.left: parent.left
            anchors.leftMargin: 12
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

        // Settings gear
        MouseArea {
            id: settingsBtn
            width: 40
            height: 40
            anchors.left: backBtn.right
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: settingsPanel.open = !settingsPanel.open

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: settingsBtn.containsMouse || settingsPanel.open
                       ? Qt.rgba(1, 1, 1, 0.08)
                       : "transparent"
                Behavior on color { ColorAnimation { duration: 150 } }

                Image {
                    anchors.centerIn: parent
                    width: 22
                    height: 22
                    source: "qrc:/assets/icons/settings.svg"
                    sourceSize.width: 22
                    sourceSize.height: 22
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    opacity: settingsBtn.containsMouse || settingsPanel.open ? 1.0 : 0.85
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }
            }
        }

        // Title (center)
        Text {
            anchors.centerIn: parent
            text: "Keyboard"
            font.family: figTreeVariable.name
            font.pixelSize: 20
            font.weight: Font.Light
            font.letterSpacing: 2
            color: "#E07826"
        }

        // Type selector (top-right)
        TypeSelector {
            id: keyboardTypeSelector
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            model: ["Synthesizer", "Electronic Keyboard", "Acoustic Piano"]
            currentIndex: 0
        }
    }

    // --- Settings pane overlay ----------------------------------------------
    // Transparent catcher — clicking outside the pane closes it.
    MouseArea {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: settingsPanel.open
        onClicked: settingsPanel.open = false
        z: 20
    }

    SettingsPanel {
        id: settingsPanel
        title: "Keyboard Settings"
        font.family: figTreeVariable.name
        
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: header.bottom
        anchors.topMargin: 12
        z: 30

        // Section: Volume
        Text {
            text: "VOLUME"
            font.family: figTreeVariable.name
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.letterSpacing: 1.5
            color: "#E07826"
        }

        SettingsSlider {
            label: "Min Volume"
            unit: "%"
            value: 10
            from: 0
            to: 100
        }

        SettingsSlider {
            label: "Max Volume"
            unit: "%"
            value: 85
            from: 0
            to: 100
        }

        // Separator
        Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.06) }

        // Section: Gesture Control
        Text {
            text: "GESTURE CONTROL"
            font.family: figTreeVariable.name
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.letterSpacing: 1.5
            color: "#E07826"
        }

        SettingsSlider {
            label: "Key Sensitivity"
            unit: "%"
            value: 65
            from: 10
            to: 100
        }

        SettingsSlider {
            label: "Octave Range"
            value: 2
            from: 1
            to: 3
            stepSize: 1
        }

        // Separator
        Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.06) }

        // Section: Playback
        Text {
            text: "PLAYBACK"
            font.family: figTreeVariable.name
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.letterSpacing: 1.5
            color: "#E07826"
        }

        SettingsToggle {
            label: "Sustain"
            checked: false
        }

        // Velocity Curve selector
        Item {
            width: parent.width
            height: 36

            Text {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: "Velocity Curve"
                font.family: figTreeVariable.name
                font.pixelSize: 12
                font.weight: Font.Medium
                font.letterSpacing: 0.5
                color: "#949AA5"
            }

            TypeSelector {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 140
                model: ["Linear", "Exponential", "Logarithmic"]
                currentIndex: 0
            }
        }
    }
}
