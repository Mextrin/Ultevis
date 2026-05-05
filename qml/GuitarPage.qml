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

    // --- Live Camera Feed ---
    Item {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        Image {
            id: cameraFeed
            anchors.fill: parent
            fillMode: Image.PreserveAspectFit
            source: "image://camera/feed"
            cache: false // Forces the UI to keep pulling the newest frame

            Timer {
                interval: 33 // 30 FPS refresh rate
                running: true
                repeat: true
                onTriggered: {
                    cameraFeed.source = "image://camera/feed?id=" + Math.random()
                }
            }
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
            onClicked: {
                appEngine.goBack()
                root.back()
            }

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
            text: "Guitar"
            font.family: figTreeVariable.name
            font.pixelSize: 20
            font.weight: Font.Light
            font.letterSpacing: 2
            color: "#E07826"
        }

        // Type selector (top-right)
        TypeSelector {
            id: guitarTypeSelector
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            model: ["Acoustic Guitar", "Clean Electric", "Distorted Electric"]
            currentIndex: 0
            onActivated: function(index) {
                if (typeof appEngine === "undefined")
                    return

                const soundIds = [2, 0, 1]
                appEngine.setGuitarSound(soundIds[index] ?? 2)
            }
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
        title: "Guitar Settings"
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
            label: "Strum Sensitivity"
            unit: "%"
            value: 60
            from: 10
            to: 100
        }

        SettingsSlider {
            label: "Strum Speed Threshold"
            unit: "%"
            value: 40
            from: 10
            to: 90
        }

        // Separator
        Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.06) }

        // Section: Instrument
        Text {
            text: "INSTRUMENT"
            font.family: figTreeVariable.name
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.letterSpacing: 1.5
            color: "#E07826"
        }

        SettingsSlider {
            label: "Number of Strings"
            value: 6
            from: 4
            to: 6
            stepSize: 2
        }

        SettingsSlider {
            label: "Capo Position"
            value: 0
            from: 0
            to: 12
            stepSize: 1
        }
    }
}
