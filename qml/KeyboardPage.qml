import QtQuick
import QtQuick.Controls
import QtMultimedia
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

    // --- Camera feed (background) -------------------------------------------
    MediaDevices {
        id: mediaDevices
        onVideoInputsChanged: {
            if (camera.active) return
            if (appEngine.cameraPermission === "granted")
                root.startCamera()
        }
    }

    readonly property int _posFrontFace: 2

    function pickCameraDevice() {
        const devices = mediaDevices.videoInputs
        if (!devices || devices.length === 0) return null

        for (let i = 0; i < devices.length; ++i) {
            const d = devices[i].description.toLowerCase()
            if (d.includes("facetime") || d.includes("built-in") || d.includes("webcam"))
                return devices[i]
        }
        for (let i = 0; i < devices.length; ++i) {
            const d = devices[i].description.toLowerCase()
            if (!d.includes("iphone") && !d.includes("ipad") && !d.includes("continuity"))
                return devices[i]
        }
        return devices[0]
    }

    property string cameraStatus: "initializing"

    Connections {
        target: appEngine
        function onCameraPermissionChanged() {
            if (appEngine.cameraPermission === "granted") root.startCamera()
            else if (appEngine.cameraPermission === "denied") root.cameraStatus = "denied"
        }
    }

    function startCamera() {
        const device = pickCameraDevice()
        if (device) { camera.cameraDevice = device; camera.active = true }
    }

    CaptureSession {
        id: captureSession
        videoOutput: videoOutput
        camera: Camera {
            id: camera
            active: false
            onActiveChanged: if (active) root.cameraStatus = "running"
        }
    }

    VideoOutput {
        id: videoOutput
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: keyboardArea.top
        fillMode: VideoOutput.PreserveAspectCrop
        transform: Scale { origin.x: videoOutput.width / 2; xScale: -1 }
    }

    Rectangle {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: keyboardArea.top
        color: "#101218"
        visible: root.cameraStatus !== "running"
        z: 1

        Text {
            anchors.centerIn: parent
            text: "Camera Feed"
            font.pixelSize: 18
            font.weight: Font.Light
            font.letterSpacing: 2
            color: Qt.rgba(1, 1, 1, 0.15)
        }
    }

    Component.onCompleted: {
        if (appEngine.cameraPermission === "granted") root.startCamera()
        else if (appEngine.cameraPermission === "denied") root.cameraStatus = "denied"
        else appEngine.requestCameraPermission()
    }

    Component.onDestruction: { camera.active = false }

    // --- Header -------------------------------------------------------------
    Item {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        z: 10

        Rectangle { anchors.fill: parent; color: Qt.rgba(0.063, 0.071, 0.094, 0.85) }

        MouseArea {
            id: backBtn
            width: 48; height: 48
            anchors.left: parent.left; anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: root.back()
            Text {
                anchors.centerIn: parent
                text: "←"; font.pixelSize: 24
                color: backBtn.containsMouse ? "#E07A26" : "#949AA5"
                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }

        Text {
            anchors.centerIn: parent
            text: "Keyboard"
            font.family: figTreeVariable.name
            font.pixelSize: 20; font.weight: Font.Light; font.letterSpacing: 2
            color: "#E07826"
        }

        Row {
            anchors.right: parent.right; anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            Text { text: "🔊"; font.pixelSize: 16; color: "#949AA5"; anchors.verticalCenter: parent.verticalCenter }
            Slider {
                id: volSlider
                width: 90; anchors.verticalCenter: parent.verticalCenter
                from: 0; to: 1; stepSize: 0.01
                value: appEngine.viewState.masterVolume
                onValueChanged: appEngine.setMasterVolume(value)
                background: Rectangle {
                    x: volSlider.leftPadding
                    y: volSlider.topPadding + volSlider.availableHeight / 2 - height / 2
                    width: volSlider.availableWidth; height: 3; radius: 2
                    color: Qt.rgba(1,1,1,0.08)
                    Rectangle { width: volSlider.visualPosition * parent.width; height: parent.height; color: "#E07A26"; radius: 2 }
                }
                handle: Rectangle {
                    x: volSlider.leftPadding + volSlider.visualPosition * (volSlider.availableWidth - width)
                    y: volSlider.topPadding + volSlider.availableHeight / 2 - height / 2
                    width: 12; height: 12; radius: 6
                    color: volSlider.pressed ? "#E07A26" : "#EBEDF0"
                    border.color: "#E07A26"; border.width: 1
                }
            }
        }
    }

    // --- Piano keyboard -----------------------------------------------------
    // Two octaves: C4 (MIDI 60) through B5 (MIDI 83)
    // White keys: 14 total  |  Black keys: 10 total
    Item {
        id: keyboardArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: Math.min(parent.height * 0.32, 180)
        z: 10

        Rectangle { anchors.fill: parent; color: "#0D0F14" }

        // White key model: [midiNote, whiteIndex]
        // C4=60 D4=62 E4=64 F4=65 G4=67 A4=69 B4=71
        // C5=72 D5=74 E5=76 F5=77 G5=79 A5=81 B5=83
        property var whiteKeys: [
            {note: 60, idx: 0},  {note: 62, idx: 1},  {note: 64, idx: 2},
            {note: 65, idx: 3},  {note: 67, idx: 4},  {note: 69, idx: 5},
            {note: 71, idx: 6},  {note: 72, idx: 7},  {note: 74, idx: 8},
            {note: 76, idx: 9},  {note: 77, idx: 10}, {note: 79, idx: 11},
            {note: 81, idx: 12}, {note: 83, idx: 13}
        ]

        // Black key model: [midiNote, position between white keys (after whiteIndex n)]
        property var blackKeys: [
            {note: 61, after: 0},  {note: 63, after: 1},
            {note: 66, after: 3},  {note: 68, after: 4},  {note: 70, after: 5},
            {note: 73, after: 7},  {note: 75, after: 8},
            {note: 78, after: 10}, {note: 80, after: 11}, {note: 82, after: 12}
        ]

        readonly property int totalWhite: 14
        readonly property real ww: width / totalWhite   // white key width
        readonly property real wh: height - 4           // white key height
        readonly property real bw: ww * 0.62
        readonly property real bh: wh * 0.60

        // White keys
        Repeater {
            model: keyboardArea.whiteKeys
            delegate: Rectangle {
                required property var modelData
                x: modelData.idx * keyboardArea.ww + 1
                y: 4
                width: keyboardArea.ww - 2
                height: keyboardArea.wh
                radius: 3
                color: pressed ? "#E07A26" : (hovered ? "#F0F0F0" : "#EBEDF0")
                border.color: "#555"; border.width: 1
                property bool hovered: false
                property bool pressed: false
                Behavior on color { ColorAnimation { duration: 80 } }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.hovered = true
                    onExited:  parent.hovered = false
                    onPressed: {
                        parent.pressed = true
                        appEngine.triggerKeyboardNote(modelData.note, 100)
                    }
                    onReleased: {
                        parent.pressed = false
                        appEngine.releaseKeyboardNote()
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom; anchors.bottomMargin: 6
                    text: modelData.note === 60 ? "C4" : modelData.note === 72 ? "C5" : ""
                    font.pixelSize: 9; color: "#888"
                }
            }
        }

        // Black keys (drawn on top)
        Repeater {
            model: keyboardArea.blackKeys
            delegate: Rectangle {
                required property var modelData
                x: (modelData.after + 1) * keyboardArea.ww - keyboardArea.bw / 2
                y: 4
                width: keyboardArea.bw
                height: keyboardArea.bh
                radius: 2
                color: pressed ? "#E07A26" : "#1A1A1A"
                border.color: pressed ? "#E07A26" : "#333"; border.width: 1
                z: 2
                property bool pressed: false
                Behavior on color { ColorAnimation { duration: 80 } }

                MouseArea {
                    anchors.fill: parent
                    onPressed: {
                        parent.pressed = true
                        appEngine.triggerKeyboardNote(modelData.note, 110)
                    }
                    onReleased: {
                        parent.pressed = false
                        appEngine.releaseKeyboardNote()
                    }
                }
            }
        }
    }

    // --- Settings pane ------------------------------------------------------
    MouseArea {
        anchors.top: header.bottom; anchors.left: parent.left
        anchors.right: parent.right; anchors.bottom: keyboardArea.top
        visible: settingsPanel.open
        onClicked: settingsPanel.open = false
        z: 20
    }

    SettingsPanel {
        id: settingsPanel
        title: "Keyboard Settings"
        font.family: figTreeVariable.name
        anchors.left: parent.left; anchors.leftMargin: 20
        anchors.top: header.bottom; anchors.topMargin: 12
        z: 30

        Text {
            text: "VOLUME"
            font.family: figTreeVariable.name; font.pixelSize: 11
            font.weight: Font.DemiBold; font.letterSpacing: 1.5; color: "#E07826"
        }
        SettingsSlider {
            label: "Master Volume"; unit: "%"
            value: appEngine.viewState.masterVolume * 100
            from: 0; to: 100
            onValueChanged: appEngine.setMasterVolume(value / 100)
        }
        Rectangle { width: parent.width; height: 1; color: Qt.rgba(1,1,1,0.06) }
        Text {
            text: "PLAYBACK"
            font.family: figTreeVariable.name; font.pixelSize: 11
            font.weight: Font.DemiBold; font.letterSpacing: 1.5; color: "#E07826"
        }
        SettingsToggle { label: "Sustain"; checked: false }
    }
}
