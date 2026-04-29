pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "components"

Item {
    id: root
    signal back()

    readonly property bool engineReady: typeof appEngine !== "undefined"
    readonly property int k1Octave: engineReady ? appEngine.topKeyboardOctave : 3
    readonly property int k2Octave: engineReady ? appEngine.bottomKeyboardOctave : 5
    property int counterFlashKeyboard: 0
    property int counterFlashDelta: 0

    function handInsideItem(item, x, y) {
        if (!item || cameraViewport.width <= 0 || cameraViewport.height <= 0)
            return false

        const point = item.mapFromItem(cameraViewport, x * cameraViewport.width, y * cameraViewport.height)
        return point.x >= 0 && point.x <= item.width && point.y >= 0 && point.y <= item.height
    }

    function leftHandInside(item) {
        return engineReady && appEngine.leftHandVisible
            && handInsideItem(item, appEngine.leftHandX, appEngine.leftHandY)
    }

    function rightHandInside(item) {
        return engineReady && appEngine.rightHandVisible
            && handInsideItem(item, appEngine.rightHandX, appEngine.rightHandY)
    }

    function anyHandInside(item) {
        return leftHandInside(item) || rightHandInside(item)
    }

    function anyThumbUpInside(item) {
        if (settingsPanel.open)
            return false

        return (engineReady && appEngine.leftHandVisible && appEngine.leftThumbUp
                && handInsideItem(item, appEngine.leftHandX, appEngine.leftHandY))
            || (engineReady && appEngine.rightHandVisible && appEngine.rightThumbUp
                && handInsideItem(item, appEngine.rightHandX, appEngine.rightHandY))
    }

    function anyThumbDownInside(item) {
        if (settingsPanel.open)
            return false

        return (engineReady && appEngine.leftHandVisible && appEngine.leftThumbDown
                && handInsideItem(item, appEngine.leftHandX, appEngine.leftHandY))
            || (engineReady && appEngine.rightHandVisible && appEngine.rightThumbDown
                && handInsideItem(item, appEngine.rightHandX, appEngine.rightHandY))
    }

    function adjustKeyboardOctave(keyboardIndex, delta) {
        if (engineReady)
            appEngine.adjustKeyboardOctave(keyboardIndex, delta)
    }

    function flashOctaveCounter(keyboardIndex, delta) {
        counterFlashKeyboard = keyboardIndex
        counterFlashDelta = delta
        counterFlashTimer.restart()
    }

    Timer {
        id: counterFlashTimer
        interval: 420
        repeat: false
        onTriggered: {
            root.counterFlashKeyboard = 0
            root.counterFlashDelta = 0
        }
    }

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
                color: Qt.rgba(0.01, 0.012, 0.018, 0.16)
            }

            Column {
                id: keyboardOverlay
                anchors.fill: parent
                spacing: 0

                Row {
                    id: octaveControls
                    width: parent.width
                    height: Math.max(58, Math.min(82, parent.height * 0.12))
                    spacing: 0

                    OctaveGroup {
                        width: parent.width / 2
                        height: parent.height
                        keyboardIndex: 1
                        title: "K1 Octave"
                        octaveValue: root.k1Octave
                    }

                    OctaveGroup {
                        width: parent.width / 2
                        height: parent.height
                        keyboardIndex: 2
                        title: "K2 Octave"
                        octaveValue: root.k2Octave
                    }
                }

                KeyboardZone {
                    width: parent.width
                    height: (parent.height - octaveControls.height) / 2
                    keyboardIndex: 1
                    label: "Keyboard Octave 1"
                }

                KeyboardZone {
                    width: parent.width
                    height: (parent.height - octaveControls.height) / 2
                    keyboardIndex: 2
                    label: "Keyboard Octave 2"
                }
            }
        }
    }

    component OctaveGroup: Item {
        id: group
        property int keyboardIndex: 1
        property string title: "K1 Octave"
        property int octaveValue: 3
        readonly property int flashDelta: root.counterFlashKeyboard === keyboardIndex ? root.counterFlashDelta : 0

        Rectangle {
            anchors.fill: parent
            color: group.flashDelta > 0 ? Qt.rgba(0.12, 0.72, 0.36, 0.42)
                  : group.flashDelta < 0 ? Qt.rgba(0.9, 0.16, 0.16, 0.42)
                  : Qt.rgba(0.015, 0.018, 0.024, 0.34)
            border.width: 1
            border.color: group.flashDelta > 0 ? "#2FE174"
                        : group.flashDelta < 0 ? "#FF4D4D"
                        : Qt.rgba(1, 1, 1, 0.32)

            Behavior on color { ColorAnimation { duration: 130 } }
            Behavior on border.color { ColorAnimation { duration: 130 } }
        }

        Item {
            anchors.fill: parent

            Text {
                anchors.centerIn: parent
                text: group.title + " " + group.octaveValue
                font.family: figTreeVariable.name
                font.pixelSize: Math.max(20, Math.min(34, parent.height * 0.48))
                font.weight: Font.Medium
                color: "#EBEDF0"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    component KeyboardZone: Rectangle {
        id: zone
        property int keyboardIndex: 1
        property string label: "Keyboard Octave"
        property bool active: root.anyHandInside(zone)
        property bool thumbUpHover: root.anyThumbUpInside(zone)
        property bool thumbDownHover: root.anyThumbDownInside(zone)
        property bool octaveGestureActive: thumbUpHover || thumbDownHover
        property int octaveGestureDelta: thumbUpHover === thumbDownHover ? 0 : (thumbUpHover ? 1 : -1)

        color: active ? Qt.rgba(0.878, 0.478, 0.149, 0.23)
                      : Qt.rgba(1, 1, 1, 0.035)
        border.width: active ? 3 : 2
        border.color: active ? "#E07A26" : Qt.rgba(1, 1, 1, 0.34)

        Behavior on color { ColorAnimation { duration: 140 } }
        Behavior on border.color { ColorAnimation { duration: 140 } }
        Behavior on border.width { NumberAnimation { duration: 140 } }

        function triggerOctaveChange(delta) {
            if (delta === 0)
                return

            root.flashOctaveCounter(keyboardIndex, delta)
            root.adjustKeyboardOctave(keyboardIndex, delta)
        }

        OctaveGestureHoldController {
            inputDelta: zone.octaveGestureDelta
            onOctaveChange: function(delta) {
                zone.triggerOctaveChange(delta)
            }
        }

        Text {
            anchors.centerIn: parent
            width: parent.width * 0.9
            text: zone.label
            font.family: figTreeVariable.name
            font.pixelSize: Math.max(30, Math.min(58, parent.height * 0.28))
            font.weight: Font.Light
            color: zone.active ? "#FFFFFF" : Qt.rgba(0.922, 0.929, 0.941, 0.72)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
            elide: Text.ElideRight
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
            onClicked: root.back()

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
            text: "Keyboard"
            font.family: figTreeVariable.name
            font.pixelSize: 20
            font.weight: Font.Light
            font.letterSpacing: 2
            color: "#E07826"
        }

        KeyboardDropdownImages {
            id: keyboardDropdownImages
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            width: 210
            onChanged: function(key) {
                if (typeof appEngine !== "undefined") {
                    const index = ["grand_piano", "organ", "flute", "harp", "violin"].indexOf(key)
                    appEngine.setKeyboardInstrument(index)
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
        title: "Settings"
        font.family: figTreeVariable.name

        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: header.bottom
        anchors.topMargin: 12
        z: 30


        Text {
    text: "MIDI OUTPUT"
    font.family: figTreeVariable.name
    font.pixelSize: 11
    font.weight: Font.DemiBold
    font.letterSpacing: 1.5
    color: "#E07826"
}

ComboBox {
    id: midiDeviceSelect
    width: parent.width
    height: 40
    model: typeof appEngine !== "undefined" ? appEngine.midiDeviceNames : ["None"]
    currentIndex: 0

    Component.onCompleted: {
        if (typeof appEngine !== "undefined") {
            let idx = find(appEngine.currentMidiDevice)
            if (idx !== -1) currentIndex = idx
        }
    }

    Connections {
        target: typeof appEngine !== "undefined" ? appEngine : null
        function onCurrentMidiDeviceChanged() {
            let idx = midiDeviceSelect.find(appEngine.currentMidiDevice)
            if (idx !== -1 && midiDeviceSelect.currentIndex !== idx)
                midiDeviceSelect.currentIndex = idx
        }
    }

    onActivated: function(index) {
        if (typeof appEngine !== "undefined")
            appEngine.selectMidiDevice(currentText)
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
    width: parent.width
    value: typeof appEngine !== "undefined" && appEngine.viewState ? (appEngine.viewState.masterVolume * 100) : 100
    onValueChanged: {
        if (typeof appEngine !== "undefined")
            appEngine.setMasterVolume(value / 100.0)
    }
}

Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.06) }


        Text {
            text: "KEYBOARD SETTINGS"
            font.family: figTreeVariable.name
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.letterSpacing: 1.5
            color: "#E07826"
        }

        SettingsToggle {
            label: "Sustain"
            checked: typeof appEngine !== "undefined" && appEngine.viewState ? appEngine.viewState.sustainPedal : false
            onCheckedChanged: {
                if (typeof appEngine !== "undefined") {
                    appEngine.setSustainPedal(checked)
                }
            }
        }
    }
}
