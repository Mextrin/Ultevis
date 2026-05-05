pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Item {
    id: root
    signal back()

    readonly property bool engineReady: typeof appEngine !== "undefined"
    readonly property bool leftHandTracked: engineReady && appEngine.leftHandVisible
    readonly property bool rightHandTracked: engineReady && appEngine.rightHandVisible
    readonly property bool leftPinchActive: engineReady && appEngine.leftPinch
    readonly property string selectedGuitarName: guitarTypeSelector.displayText

    FontLoader {
        id: figTreeVariable
        source: "../assets/fonts/Figtree-VariableFont_wght.ttf"
    }

    Rectangle {
        anchors.fill: parent
        color: "#101218"
    }

    Item {
        id: stage
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        readonly property real cameraAspect: 4 / 3
        readonly property real fittedWidth: height <= 0 ? 0 : (width / height > cameraAspect ? height * cameraAspect : width)
        readonly property real fittedHeight: height <= 0 ? 0 : (width / height > cameraAspect ? height : width / cameraAspect)

        Rectangle {
            anchors.fill: parent
            color: "#0A0C10"
        }

        Item {
            id: cameraViewport
            width: stage.fittedWidth
            height: stage.fittedHeight
            anchors.centerIn: parent
            clip: true

            Image {
                id: cameraFeed
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                source: "image://camera/feed"
                cache: false
                smooth: true
                mipmap: true

                Timer {
                    interval: 33
                    running: true
                    repeat: true
                    onTriggered: {
                        cameraFeed.source = "image://camera/feed?id=" + Math.random()
                    }
                }
            }

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(0.01, 0.012, 0.018, 0.30) }
                    GradientStop { position: 0.58; color: Qt.rgba(0.01, 0.012, 0.018, 0.08) }
                    GradientStop { position: 1.0; color: Qt.rgba(0.01, 0.012, 0.018, 0.34) }
                }
            }

            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 22
                spacing: 18

                Rectangle {
                    width: Math.min(parent.width * 0.56, 430)
                    height: 168
                    radius: 18
                    color: Qt.rgba(0.055, 0.065, 0.09, 0.78)
                    border.width: 1
                    border.color: Qt.rgba(1, 1, 1, 0.10)

                    Rectangle {
                        width: parent.width
                        height: 3
                        radius: 2
                        anchors.top: parent.top
                        color: "#E07826"
                    }

                    Column {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 12

                        Text {
                            text: selectedGuitarName
                            font.family: figTreeVariable.name
                            font.pixelSize: 26
                            font.weight: Font.Medium
                            color: "#F4F5F6"
                        }

                        Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            text: "Pinch with your left hand to shape the chord. Strum with an open right hand. Hold a closed fist to block strums."
                            font.family: figTreeVariable.name
                            font.pixelSize: 14
                            lineHeight: 1.28
                            color: Qt.rgba(0.92, 0.93, 0.94, 0.86)
                        }

                        Row {
                            spacing: 10

                            GestureChip {
                                label: "Chord Hand"
                                active: leftHandTracked
                                activeText: "Tracked"
                                inactiveText: "Show left hand"
                            }

                            GestureChip {
                                label: "Strum Hand"
                                active: rightHandTracked
                                activeText: "Tracked"
                                inactiveText: "Show right hand"
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    width: Math.min(parent.width * 0.30, 250)
                    height: 168
                    radius: 18
                    color: Qt.rgba(0.055, 0.065, 0.09, 0.70)
                    border.width: 1
                    border.color: Qt.rgba(1, 1, 1, 0.10)

                    Column {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 12

                        Text {
                            text: "Live Status"
                            font.family: figTreeVariable.name
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.3
                            color: "#E07826"
                        }

                        StatusRow {
                            label: "Fret hand"
                            value: leftHandTracked ? "Tracked" : "Searching"
                            active: leftHandTracked
                        }

                        StatusRow {
                            label: "Chord pinch"
                            value: leftPinchActive ? "Active" : "Idle"
                            active: leftPinchActive
                        }

                        StatusRow {
                            label: "Strum hand"
                            value: rightHandTracked ? "Tracked" : "Searching"
                            active: rightHandTracked
                        }

                        Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            text: "Upstrum and downstrum are enabled. Closed fist suppresses both."
                            font.family: figTreeVariable.name
                            font.pixelSize: 12
                            color: Qt.rgba(0.89, 0.91, 0.94, 0.72)
                        }
                    }
                }
            }
        }
    }

    component GestureChip: Rectangle {
        property string label: "Hand"
        property bool active: false
        property string activeText: "Ready"
        property string inactiveText: "Waiting"

        radius: 999
        height: 34
        width: chipText.implicitWidth + 24
        color: active ? Qt.rgba(0.878, 0.478, 0.149, 0.22) : Qt.rgba(1, 1, 1, 0.05)
        border.width: 1
        border.color: active ? Qt.rgba(0.878, 0.478, 0.149, 0.75) : Qt.rgba(1, 1, 1, 0.10)

        Text {
            id: chipText
            anchors.centerIn: parent
            text: label + "  " + (active ? activeText : inactiveText)
            font.family: figTreeVariable.name
            font.pixelSize: 12
            font.weight: Font.Medium
            color: active ? "#FFD9B2" : Qt.rgba(0.92, 0.93, 0.94, 0.76)
        }
    }

    component StatusRow: Item {
        property string label: "Status"
        property string value: "Idle"
        property bool active: false

        width: parent.width
        height: 24

        Text {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            text: label
            font.family: figTreeVariable.name
            font.pixelSize: 13
            color: Qt.rgba(0.90, 0.92, 0.94, 0.80)
        }

        Rectangle {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: 22
            width: statusText.implicitWidth + 20
            radius: 999
            color: active ? Qt.rgba(0.17, 0.70, 0.41, 0.20) : Qt.rgba(1, 1, 1, 0.06)
            border.width: 1
            border.color: active ? Qt.rgba(0.26, 0.86, 0.53, 0.70) : Qt.rgba(1, 1, 1, 0.12)

            Text {
                id: statusText
                anchors.centerIn: parent
                text: value
                font.family: figTreeVariable.name
                font.pixelSize: 12
                font.weight: Font.Medium
                color: active ? "#D8FFE8" : Qt.rgba(0.92, 0.93, 0.94, 0.72)
            }
        }
    }

    Item {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        z: 10

        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0.063, 0.071, 0.094, 0.75)
        }

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

        Text {
            anchors.centerIn: parent
            text: "Guitar"
            font.family: figTreeVariable.name
            font.pixelSize: 20
            font.weight: Font.Light
            font.letterSpacing: 2
            color: "#E07826"
        }

        TypeSelector {
            id: guitarTypeSelector
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            width: 210
            model: ["Acoustic Guitar", "Clean Electric", "Distorted Electric"]
            currentIndex: 0
            onActivated: function(index) {
                if (typeof appEngine !== "undefined") {
                    const soundId = [2, 0, 1][index]
                    appEngine.setGuitarSound(soundId)
                }
            }
        }
    }

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
        title: "Guitar Guide"
        font.family: figTreeVariable.name

        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: header.bottom
        anchors.topMargin: 12
        z: 30

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: "Use your left hand to hold the chord and your right hand to strum through the strings."
            font.family: figTreeVariable.name
            font.pixelSize: 14
            color: "#EBEDF0"
        }

        Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.06) }

        Column {
            width: parent.width
            spacing: 10

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: "1. Show your left hand to the camera and pinch to change or hold the chord."
                font.family: figTreeVariable.name
                font.pixelSize: 13
                color: Qt.rgba(0.90, 0.92, 0.94, 0.82)
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: "2. Keep your right hand open and move down for a downstrum or up for an upstrum."
                font.family: figTreeVariable.name
                font.pixelSize: 13
                color: Qt.rgba(0.90, 0.92, 0.94, 0.82)
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: "3. Hold a closed fist whenever you want to block accidental strums."
                font.family: figTreeVariable.name
                font.pixelSize: 13
                color: Qt.rgba(0.90, 0.92, 0.94, 0.82)
            }
        }
    }
}
