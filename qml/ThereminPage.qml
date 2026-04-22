import QtQuick
import QtQuick.Controls
import QtMultimedia
import "components"

Item {
    id: root
    signal back()

    // UI state — initialised from persisted ViewState and kept in sync with backend
    property string waveShape:   appEngine.viewState.thereminWaveform
    property real   masterVolume: appEngine.viewState.masterVolume
    property real   volumeFloor: appEngine.viewState.thereminVolumeFloor
    property real   freqMin:     appEngine.viewState.thereminFreqMin
    property real   freqMax:     appEngine.viewState.thereminFreqMax

    FontLoader {
        id: figTreeVariable
        source: "../assets/fonts/Figtree-VariableFont_wght.ttf"
    }

    // Dark background (shown before the camera feed is ready or if permission is denied).
    Rectangle {
        anchors.fill: parent
        color: "#101218"
    }

    // --- Camera feed ---------------------------------------------------------
    // Picks the first front-facing camera, falling back to the default video input.
    MediaDevices {
        id: mediaDevices
        // After permission is granted, macOS populates videoInputs asynchronously.
        // Retry starting the camera when the list changes.
        onVideoInputsChanged: {
            console.log("ThereminPage: videoInputs changed, count =", videoInputs.length)
            if (camera.active) return
            if (appEngine.cameraPermission === "granted") {
                root.startCamera()
            }
        }
    }

    // QCameraDevice::Position enum values (not directly accessible as
    // a QML namespace, so use the raw ints).
    readonly property int _posFrontFace: 2

    function pickCameraDevice() {
        const devices = mediaDevices.videoInputs
        console.log("ThereminPage: pickCameraDevice; videoInputs.length =", devices ? devices.length : 0)
        if (!devices || devices.length === 0) return null
        for (let i = 0; i < devices.length; ++i)
            console.log("  device", i, "desc=", devices[i].description, "position=", devices[i].position)

        // 1. Prefer built-in FaceTime / webcam by name
        for (let i = 0; i < devices.length; ++i) {
            const d = devices[i].description.toLowerCase()
            if (d.includes("facetime") || d.includes("built-in") || d.includes("webcam"))
                return devices[i]
        }
        // 2. Skip Continuity Camera (iPhone / iPad)
        for (let i = 0; i < devices.length; ++i) {
            const d = devices[i].description.toLowerCase()
            if (!d.includes("iphone") && !d.includes("ipad") && !d.includes("continuity"))
                return devices[i]
        }
        return devices[0]
    }

    property string cameraStatus: "initializing"   // "initializing" | "running" | "no-device" | "error" | "denied"
    property string cameraErrorText: ""

    // React to the app-level camera permission outcome.
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
        console.log("ThereminPage: startCamera(); permission =", appEngine.cameraPermission)
        const device = pickCameraDevice()
        if (device) {
            console.log("ThereminPage: starting camera with device", device.description)
            camera.cameraDevice = device
            camera.active = true
        } else {
            // videoInputs may still be empty right after permission is granted.
            // We'll retry from onVideoInputsChanged when the list is populated.
            console.log("ThereminPage: no camera device yet; waiting for videoInputsChanged")
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
                console.log("ThereminPage: camera error:", error, errorString)
            }
            onActiveChanged: {
                console.log("ThereminPage: camera.active =", active)
                if (active) {
                    root.cameraStatus = "running"
                    appEngine.connectVideoSink(videoOutput.videoSink)
                }
            }
            onCameraDeviceChanged: {
                console.log("ThereminPage: camera.cameraDevice changed to", cameraDevice.description)
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
        // Mirror horizontally so the feed reads like a mirror (natural for a front camera).
        transform: Scale { origin.x: videoOutput.width / 2; xScale: -1 }
    }

    // Fallback overlay when the camera isn't running (permission denied, no device, etc.).
    Rectangle {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: "#101218"
        visible: root.cameraStatus !== "running"
        z: 5

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
                            return "Connect a camera and reopen the Theremin page."
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
        // If we already have camera permission, start right away.
        // Otherwise, ask — the system prompt will appear and the
        // Connections handler above will start the camera on "granted".
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

    // --- Hand detector status banner -----------------------------------------
    Rectangle {
        id: detectorBanner
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 36
        color: Qt.rgba(0.878, 0.478, 0.149, 0.18)
        visible: root.cameraStatus === "running" && !appEngine.handDetectorRunning
        z: 6

        Text {
            anchors.centerIn: parent
            text: "Hand detector not running — no gesture input. Run:  source venv/bin/activate && pip install mediapipe opencv-python"
            font.pixelSize: 12
            font.family: figTreeVariable.name
            color: "#E07A26"
        }
    }

    // --- Dotted guide line (30% from the left, full main-area height) --------
    Canvas {
        id: guideLine
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        // Only the area at x = 30% needs redrawing, but Canvas is cheap here.
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.save()
            ctx.strokeStyle = Qt.rgba(0.922, 0.929, 0.941, 0.8) // #EBEDF0 @ 80%
            ctx.lineWidth = 6
            ctx.setLineDash([6, 8])
            const x = Math.round(width * 0.30) + 0.5
            ctx.beginPath()
            ctx.moveTo(x, 0)
            ctx.lineTo(x, height)
            ctx.stroke()
            ctx.restore()
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
            onClicked: settingsPane.open = !settingsPane.open

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: settingsBtn.containsMouse || settingsPane.open
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
                    opacity: settingsBtn.containsMouse || settingsPane.open ? 1.0 : 0.85
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }
            }
        }

        // Title (center)
        Text {
            anchors.centerIn: parent
            text: "Theremin"
            font.family: figTreeVariable.name
            font.pixelSize: 20
            font.weight: Font.Light
            font.letterSpacing: 2
            color: "#E07826"
        }

        // Master volume slider
        Row {
            anchors.right: waveDropdown.left
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Text {
                text: "\uD83D\uDD0A"   // 🔊
                font.pixelSize: 16
                color: "#949AA5"
                anchors.verticalCenter: parent.verticalCenter
            }

            Slider {
                id: volumeSlider
                width: 90
                anchors.verticalCenter: parent.verticalCenter
                from: 0; to: 1; stepSize: 0.01
                value: root.masterVolume
                onValueChanged: {
                    root.masterVolume = value
                    appEngine.setMasterVolume(value)
                }
                background: Rectangle {
                    x: volumeSlider.leftPadding
                    y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                    width: volumeSlider.availableWidth; height: 3; radius: 2
                    color: Qt.rgba(1,1,1,0.08)
                    Rectangle {
                        width: volumeSlider.visualPosition * parent.width
                        height: parent.height; color: "#E07A26"; radius: 2
                    }
                }
                handle: Rectangle {
                    x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                    y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                    width: 12; height: 12; radius: 6
                    color: volumeSlider.pressed ? "#E07A26" : "#EBEDF0"
                    border.color: "#E07A26"; border.width: 1
                }
            }
        }

        // Wave dropdown (far right)
        WaveShapeDropdown {
            id: waveDropdown
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            value: root.waveShape
            font.family: figTreeVariable.name
            onChanged: function(v) {
                root.waveShape = v
                appEngine.setThereminWaveform(v)
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
        visible: settingsPane.open
        onClicked: settingsPane.open = false
        z: 20
    }

    SettingsPane {
        id: settingsPane
        property bool open: false

        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: header.bottom
        anchors.topMargin: 12
        width: 300
        z: 30

        font.family: figTreeVariable.name
        masterVolume: root.masterVolume
        volumeFloor: root.volumeFloor
        freqMin: root.freqMin
        freqMax: root.freqMax

        onMasterVolumeChanged: {
            root.masterVolume = masterVolume
            appEngine.setMasterVolume(masterVolume)
        }
        onVolumeFloorChanged: {
            root.volumeFloor = volumeFloor
            appEngine.setThereminVolumeFloor(volumeFloor)
        }
        onFreqMinChanged: {
            root.freqMin = freqMin
            appEngine.setThereminFreqMin(freqMin)
        }
        onFreqMaxChanged: {
            root.freqMax = freqMax
            appEngine.setThereminFreqMax(freqMax)
        }
        onClose: open = false

        visible: open
        opacity: open ? 1.0 : 0.0
        scale: open ? 1.0 : 0.96
        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
        Behavior on scale   { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
    }
}
