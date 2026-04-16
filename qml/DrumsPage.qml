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

    // Webcam placeholder (behind drum overlay)
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
            text: "Drums"
            font.family: figTreeVariable.name
            font.pixelSize: 20
            font.weight: Font.Light
            font.letterSpacing: 2
            color: "#E07826"
        }

        // Type selector (top-right)
        
    }

    // --- Drum kit overlay (zone indicators) ----------------------------------
    //
    // Layout rationale (drummer's perspective, optimised for webcam gesture control):
    //
    //   The webcam sees the player from the front, so left/right are mirrored.
    //   Zones are sized and spaced so that large hand movements map to distinct
    //   regions with minimal accidental cross-triggering.
    //
    //   • Cymbals (Crash, Ride) sit at the top — arms reach upward to hit them,
    //     which is the natural gesture and matches real-kit ergonomics.
    //   • Toms sit in a centre band just below the cymbals — quick lateral
    //     movements choose between them. Floor Tom is lower-right, matching
    //     its real-world position relative to the drummer.
    //   • Hi-Hat is far left, mid-height — the most frequently struck element
    //     gets a generous zone in the dominant-hand's resting region.
    //   • Snare is placed on a clean diagonal between Hi-Hat (upper-left) and
    //     Kick (lower-centre), so a natural downward sweeping gesture crosses
    //     Hi-Hat → Snare → Kick in sequence — the most common rudiment path.
    //   • Kick is at the very bottom centre — a downward punch or stomp gesture
    //     triggers it, keeping it distinct from everything above.
    //
    Item {
        id: drumKit
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 24
        z: 5

        // --- Row 1: Cymbals (top) -------------------------------------------

        // Crash — top-left (large, easy target for dramatic gestures)
        DrumPad {
            id: crashPad
            name: "Crash"
            x: 0
            y: 0
            width: parent.width * 0.24
            height: parent.height * 0.26
        }

        // Ride — top-right (mirrors Crash on the opposite side)
        DrumPad {
            id: ridePad
            name: "Ride"
            x: parent.width - width
            y: 0
            width: parent.width * 0.24
            height: parent.height * 0.26
        }

        // --- Row 2: Toms (upper-centre band) ---------------------------------

        // Tom 1 — left-of-centre
        DrumPad {
            id: tom1Pad
            name: "High-Tom"
            x: parent.width * 0.30
            y: parent.height * 0.06
            width: parent.width * 0.17
            height: parent.height * 0.22
        }

        // Tom 2 — right-of-centre
        DrumPad {
            id: tom2Pad
            name: "Low-Tom"
            x: parent.width * 0.53
            y: parent.height * 0.06
            width: parent.width * 0.17
            height: parent.height * 0.22
        }

        // --- Row 3: Hi-Hat, Snare, Floor Tom (mid band) ----------------------

        // Hi-Hat — far left, mid-height
        DrumPad {
            id: hiHatPad
            name: "Hi-Hat"
            x: 0
            y: parent.height * 0.34
            width: parent.width * 0.22
            height: parent.height * 0.22
        }

        // Snare — on the diagonal between Hi-Hat and Kick
        // Horizontally centred between Hi-Hat's right edge and screen centre,
        // vertically halfway between Hi-Hat and Kick.
        DrumPad {
            id: snarePad
            name: "Snare"
            x: parent.width * 0.28
            y: parent.height * 0.44
            width: parent.width * 0.20
            height: parent.height * 0.22
        }

        // Floor Tom — right side, below Ride (real-kit position)
        DrumPad {
            id: floorTomPad
            name: "Floor Tom"
            x: parent.width - width
            y: parent.height * 0.44
            width: parent.width * 0.22
            height: parent.height * 0.26
        }

        // --- Row 4: Kick (bottom-centre) -------------------------------------

        // Kick — very bottom, centred, wide target for downward punches
        DrumPad {
            id: kickPad
            name: "Kick"
            x: parent.width * 0.30
            y: parent.height * 0.73
            width: parent.width * 0.40
            height: parent.height * 0.24
        }
    }

    // --- Settings pane overlay ----------------------------------------------
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
        title: "Drums Settings"
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
            label: "Hit Sensitivity"
            unit: "%"
            value: 55
            from: 10
            to: 100
        }

        SettingsSlider {
            label: "Hit Speed Threshold"
            unit: "%"
            value: 35
            from: 10
            to: 90
        }

        SettingsToggle {
            label: "Double-Hit Detection"
            checked: true
        }

        // Separator
        Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.06) }

        // Section: Kit
        Text {
            text: "KIT"
            font.family: figTreeVariable.name
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.letterSpacing: 1.5
            color: "#E07826"
        }

        SettingsToggle {
            label: "Show Zone Labels"
            checked: true
        }

        SettingsSlider {
            label: "Zone Opacity"
            unit: "%"
            value: 60
            from: 10
            to: 100
        }
    }

    // --- Inline DrumPad component -------------------------------------------
    component DrumPad: Rectangle {
        id: pad
        property string name: ""
        property bool padHovered: padMouse.containsMouse

        radius: 10
        color: padHovered ? Qt.rgba(0.878, 0.478, 0.149, 0.14) : Qt.rgba(1, 1, 1, 0.04)
        border.width: 1
        border.color: padHovered ? Qt.rgba(0.878, 0.478, 0.149, 0.5) : Qt.rgba(1, 1, 1, 0.08)

        Behavior on color { ColorAnimation { duration: 180 } }
        Behavior on border.color { ColorAnimation { duration: 180 } }

        transform: Scale {
            origin.x: pad.width / 2
            origin.y: pad.height / 2
            xScale: pad.padHovered ? 1.02 : 1.0
            yScale: pad.padHovered ? 1.02 : 1.0
            Behavior on xScale { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
            Behavior on yScale { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
        }

        Text {
            anchors.centerIn: parent
            text: pad.name
            font.family: figTreeVariable.name
            font.pixelSize: 14
            font.weight: Font.Medium
            font.letterSpacing: 1
            color: pad.padHovered ? "#E07A26" : "#EBEDF0"
            opacity: pad.padHovered ? 1.0 : 0.7
            Behavior on color { ColorAnimation { duration: 180 } }
            Behavior on opacity { NumberAnimation { duration: 180 } }
        }

        MouseArea {
            id: padMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
        }
    }
}
