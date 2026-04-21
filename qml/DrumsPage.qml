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

    // Picks the first front-facing camera, falling back to the default video input.
    MediaDevices {
        id: mediaDevices
        onVideoInputsChanged: {
            console.log("DrumsPage: videoInputs changed, count =", videoInputs.length)
            if (camera.active) return
            if (appEngine.cameraPermission === "granted") {
                root.startCamera()
            }
        }
    }

    readonly property int _posFrontFace: 2

    function pickCameraDevice() {
        const devices = mediaDevices.videoInputs
        console.log("DrumsPage: pickCameraDevice; videoInputs.length =", devices ? devices.length : 0)
        if (!devices || devices.length === 0) return null
        for (let i = 0; i < devices.length; ++i) {
            if (devices[i].position === _posFrontFace) return devices[i]
        }
        return devices[0]
    }

    property string cameraStatus: "initializing"
    property string cameraErrorText: ""

    Connections {
        target: appEngine
        function onCameraPermissionChanged() {
            if (appEngine.cameraPermission === "granted") {
                root.startCamera()
            } else if (appEngine.cameraPermission === "denied") {
                root.cameraStatus = "denied"
            }
        }
    }

    function startCamera() {
        console.log("DrumsPage: startCamera(); permission =", appEngine.cameraPermission)
        const device = pickCameraDevice()
        if (device) {
            camera.cameraDevice = device
            camera.active = true
        } else {
            console.log("DrumsPage: no camera device yet; waiting for videoInputsChanged")
        }
    }

    CaptureSession {
        id: captureSession
        videoOutput: videoOutput
        camera: Camera {
            id: camera
            active: false
            onErrorOccurred: function(error, errorString) {
                root.cameraStatus = "error"
                root.cameraErrorText = errorString
                console.log("DrumsPage: camera error:", error, errorString)
            }
            onActiveChanged: {
                console.log("DrumsPage: camera.active =", active)
                if (active) root.cameraStatus = "running"
            }
        }
    }

    VideoOutput {
        id: videoOutput
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        fillMode: VideoOutput.PreserveAspectCrop
        transform: Scale { origin.x: videoOutput.width / 2; xScale: -1 }
    }

    // Fallback overlay when the camera isn't running.
    Rectangle {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: "#101218"
        visible: root.cameraStatus !== "running"
        z: 1

        Column {
            anchors.centerIn: parent
            spacing: 10
            width: Math.min(parent.width - 80, 520)

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "\u25A1"
                font.pixelSize: 44
                color: "#949AA5"
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: {
                    switch (root.cameraStatus) {
                        case "no-device": return "No camera detected"
                        case "denied":    return "Camera access denied"
                        case "error":     return "Camera unavailable"
                        default:          return "Waiting for camera\u2026"
                    }
                }
                font.family: figTreeVariable.name
                font.pixelSize: 18
                font.weight: Font.Medium
                color: "#EBEDF0"
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: {
                    switch (root.cameraStatus) {
                        case "error":
                            return root.cameraErrorText + "\n\nOn macOS, check System Settings \u2192 Privacy & Security \u2192 Camera and grant Airchestra access."
                        case "denied":
                            return "Open System Settings \u2192 Privacy & Security \u2192 Camera and enable Airchestra, then restart the app."
                        case "no-device":
                            return "Connect a camera and reopen the Drums page."
                        default:
                            return "Grant camera access in the system prompt to see the live feed."
                    }
                }
                font.family: figTreeVariable.name
                font.pixelSize: 13
                color: "#949AA5"
                lineHeight: 1.3
            }
        }
    }

    Component.onCompleted: {
        if (appEngine.cameraPermission === "granted") {
            root.startCamera()
        } else if (appEngine.cameraPermission === "denied") {
            root.cameraStatus = "denied"
        } else {
            appEngine.requestCameraPermission()
        }
    }

    Component.onDestruction: {
        camera.active = false
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
        anchors.margins: 16
        z: 5

        // --- Row 1: Cymbals (top) -------------------------------------------

        // Crash — top-left, wide flat cymbal shape
        DrumPad {
            id: crashPad
            name: "Crash"
            drumImage: "qrc:/assets/drums/crash.png"
            x: 0
            y: parent.height * 0.08
            width: parent.width * 0.26
            height: parent.height * 0.16
        }

        // Ride — top-right, wide flat cymbal shape
        DrumPad {
            id: ridePad
            name: "Ride"
            drumImage: "qrc:/assets/drums/crash.png"
            x: parent.width - width
            y: parent.height * 0.08
            width: parent.width * 0.26
            height: parent.height * 0.16
        }

        // --- Row 2: Toms (upper-centre band) ---------------------------------

        // High-Tom — small circle, tucked between crash and centre
        DrumPad {
            id: tom1Pad
            name: "High-Tom"
            drumImage: "qrc:/assets/drums/drum.png"
            x: parent.width * 0.30
            y: parent.height * 0.06
            width: parent.width * 0.14
            height: parent.height * 0.17
        }

        // Low-Tom — small circle, tucked between centre and ride
        DrumPad {
            id: tom2Pad
            name: "Low-Tom"
            drumImage: "qrc:/assets/drums/drum.png"
            x: parent.width * 0.56
            y: parent.height * 0.06
            width: parent.width * 0.14
            height: parent.height * 0.17
        }

        // --- Row 3: Hi-Hat, Snare, Floor Tom (mid band) ----------------------

        // Closed Hi-Hat — wide flat cymbal, left side mid-band
        DrumPad {
            id: closedhiHatPad
            name: "Closed Hi-Hat"
            drumImage: "qrc:/assets/drums/crash.png"
            x: 0
            y: parent.height * 0.30
            width: parent.width * 0.23
            height: parent.height * 0.12
        }

        // Open Hi-Hat — wide flat cymbal, stacked directly below closed
        DrumPad {
            id: openhiHatPad
            name: "Open Hi-Hat"
            drumImage: "qrc:/assets/drums/crash.png"
            x: 0
            y: parent.height * 0.44
            width: parent.width * 0.23
            height: parent.height * 0.12
        }

        // Snare — medium circle, center-left (GarageBand: prominent hit zone)
        DrumPad {
            id: snarePad
            name: "Snare"
            drumImage: "qrc:/assets/drums/drum.png"
            x: parent.width * 0.27
            y: parent.height * 0.30
            width: parent.width * 0.22
            height: parent.height * 0.24
        }

        // Floor Tom — larger circle than rack toms, right side
        DrumPad {
            id: floorTomPad
            name: "Floor Tom"
            drumImage: "qrc:/assets/drums/drum.png"
            x: parent.width - width
            y: parent.height * 0.28
            width: parent.width * 0.27
            height: parent.height * 0.28
        }

        // --- Row 4: Kick (bottom-centre) -------------------------------------

        // Kick — widest element, bottom-centre (GarageBand: large bass drum)
        DrumPad {
            id: kickPad
            name: "Kick"
            drumImage: "qrc:/assets/drums/drum.png"
            x: parent.width * 0.28
            y: parent.height * 0.65
            width: parent.width * 0.44
            height: parent.height * 0.22
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
        property string drumImage: ""
        property bool padHovered: padMouse.containsMouse

        radius: 10
        clip: true
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

        // Drum image — fills the pad
        Image {
            anchors.fill: parent
            source: pad.drumImage
            fillMode: Image.PreserveAspectCrop
            smooth: true
            visible: pad.drumImage !== ""
            opacity: pad.padHovered ? 1.0 : 0.85
            Behavior on opacity { NumberAnimation { duration: 180 } }
        }

        // Small label at the bottom
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 4
            text: pad.name
            font.family: figTreeVariable.name
            font.pixelSize: 10
            font.weight: Font.Medium
            font.letterSpacing: 0.5
            color: pad.padHovered ? "#E07A26" : "#EBEDF0"
            opacity: pad.padHovered ? 1.0 : 0.6
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
