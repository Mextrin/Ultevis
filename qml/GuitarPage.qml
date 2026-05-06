pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "components"

Item {
    id: root
    signal back()

    readonly property bool engineReady: typeof appEngine !== "undefined"

    // ── Neck position state ──────────────────────────────────────────────────
    property int  neckPosition:   0   // 0 = frets 1–7 · 1 = frets 8–15 · 2 = frets 16–24
    property int  neckFlashDelta: 0   // +1 = green (thumb up) · -1 = red (thumb down) · 0 = idle
    property bool strumFlash:     false

    // Rising-edge + cooldown state for neck gestures
    property bool prevThumbUp:   false
    property bool prevThumbDown: false
    property bool neckCooldown:  false

    // ── Helper functions (identical to KeyboardPage) ─────────────────────────
    function handInsideItem(item, x, y) {
        if (!item || cameraViewport.width <= 0 || cameraViewport.height <= 0)
            return false
        const pt = item.mapFromItem(cameraViewport,
                                    x * cameraViewport.width,
                                    y * cameraViewport.height)
        return pt.x >= 0 && pt.x <= item.width && pt.y >= 0 && pt.y <= item.height
    }

    function leftHandInside(item) {
        return engineReady && appEngine.leftHandVisible
            && handInsideItem(item, appEngine.leftHandX, appEngine.leftHandY)
    }

    function rightHandInside(item) {
        return engineReady && appEngine.rightHandVisible
            && handInsideItem(item, appEngine.rightHandX, appEngine.rightHandY)
    }

    function anyThumbUpInside(item) {
        return (engineReady && appEngine.leftHandVisible  && appEngine.leftThumbUp
                && handInsideItem(item, appEngine.leftHandX,  appEngine.leftHandY))
            || (engineReady && appEngine.rightHandVisible && appEngine.rightThumbUp
                && handInsideItem(item, appEngine.rightHandX, appEngine.rightHandY))
    }

    function anyThumbDownInside(item) {
        return (engineReady && appEngine.leftHandVisible  && appEngine.leftThumbDown
                && handInsideItem(item, appEngine.leftHandX,  appEngine.leftHandY))
            || (engineReady && appEngine.rightHandVisible && appEngine.rightThumbDown
                && handInsideItem(item, appEngine.rightHandX, appEngine.rightHandY))
    }

    // ── Timers ───────────────────────────────────────────────────────────────
    Timer {
        id: neckFlashTimer
        interval: 420
        repeat:   false
        onTriggered: root.neckFlashDelta = 0
    }

    Timer {
        id: strumFlashTimer
        interval: 150
        repeat:   false
        onTriggered: root.strumFlash = false
    }

    // Neck cooldown — 700 ms between neck position changes to prevent double-fire
    Timer {
        id: neckCooldownTimer
        interval: 700
        repeat:   false
        onTriggered: root.neckCooldown = false
    }

    // ── Neck gesture detection (rising-edge + cooldown, same pattern as strum) ─
    // Detection zone = entire right half so the user doesn't need to hit a tiny area.
    Connections {
        target: root.engineReady ? appEngine : null
        function onHandStateChanged() {
            if (!root.engineReady) return

            const up   = root.anyThumbUpInside(rightHalf)
            const down = root.anyThumbDownInside(rightHalf)

            if (!root.neckCooldown) {
                if (up && !root.prevThumbUp) {
                    root.neckPosition   = Math.min(2, root.neckPosition + 1)
                    root.neckFlashDelta = +1
                    neckFlashTimer.restart()
                    root.neckCooldown = true
                    neckCooldownTimer.restart()
                } else if (down && !root.prevThumbDown) {
                    root.neckPosition   = Math.max(0, root.neckPosition - 1)
                    root.neckFlashDelta = -1
                    neckFlashTimer.restart()
                    root.neckCooldown = true
                    neckCooldownTimer.restart()
                }
            }

            root.prevThumbUp   = up
            root.prevThumbDown = down
        }
    }

    // ── Fonts / background ───────────────────────────────────────────────────
    FontLoader {
        id: figTreeVariable
        source: "../assets/fonts/Figtree-VariableFont_wght.ttf"
    }

    Rectangle {
        anchors.fill: parent
        color: "#101218"
    }

    // ── Stage (fills below header — identical pattern to KeyboardPage) ────────
    Item {
        id: stage
        anchors.top:    header.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: parent.bottom

        readonly property real cameraAspect:  4 / 3
        readonly property real fittedWidth:   height <= 0 ? 0
            : (width / height > cameraAspect ? height * cameraAspect : width)
        readonly property real fittedHeight:  height <= 0 ? 0
            : (width / height > cameraAspect ? height : width / cameraAspect)

        Rectangle {
            anchors.fill: parent
            color: "#0A0C10"
        }

        // ── Camera viewport (4:3, centred) ───────────────────────────────────
        Item {
            id: cameraViewport
            width:  stage.fittedWidth
            height: stage.fittedHeight
            anchors.centerIn: parent
            clip: true

            Image {
                id: cameraFeed
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                source:   "image://camera/feed"
                cache:    false
                smooth:   true
                mipmap:   true

                Timer {
                    interval: 33
                    running:  true
                    repeat:   true
                    onTriggered: cameraFeed.source = "image://camera/feed?id=" + Math.random()
                }
            }

            // Subtle dark tint
            Rectangle {
                anchors.fill: parent
                color: Qt.rgba(0.01, 0.012, 0.018, 0.16)
            }

            // ── Guitar overlay ───────────────────────────────────────────────
            Item {
                id: guitarOverlay
                anchors.fill: parent

                // Right half: NeckCounter (top) → FretboardBars → StrumZone (bottom)
                Item {
                    id: rightHalf
                    x:      parent.width * 0.50
                    y:      0
                    width:  parent.width  * 0.50
                    height: parent.height

                    // Neck position counter — large, takes top 42% of right half
                    NeckCounter {
                        id: neckCounter
                        anchors.top:   parent.top
                        anchors.left:  parent.left
                        anchors.right: parent.right
                        height: parent.height * 0.42
                    }

                    // Strum zone — anchored to the bottom
                    StrumZone {
                        id: strumZone
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        anchors.bottom: parent.bottom
                        height: parent.height * 0.50
                    }

                    // Fretboard — sits between counter and strum zone, natural aspect ratio
                    FretboardBars {
                        anchors.left:         parent.left
                        anchors.leftMargin:   6
                        anchors.right:        parent.right
                        anchors.rightMargin:  6
                        anchors.bottom:       strumZone.top
                        anchors.bottomMargin: 4
                        height: width * (218.0 / 2090.0)
                    }
                }
            }

            // ── Hand position dots ────────────────────────────────────────────
            Rectangle {
                visible: root.engineReady && appEngine.leftHandVisible
                width: 28; height: 28; radius: 14
                color:        root.engineReady && appEngine.leftPinch ? "#e07826" : "#deab84"
                border.color: "#FFFFFF"
                border.width: 2
                x: root.engineReady ? appEngine.leftHandX  * cameraViewport.width  - 14 : 0
                y: root.engineReady ? appEngine.leftHandY  * cameraViewport.height - 14 : 0
                z: 10
            }

            Rectangle {
                visible: root.engineReady && appEngine.rightHandVisible
                width: 28; height: 28; radius: 14
                color:        root.engineReady && appEngine.rightPinch ? "#e07826" : "#deab84"
                border.color: "#FFFFFF"
                border.width: 2
                x: root.engineReady ? appEngine.rightHandX * cameraViewport.width  - 14 : 0
                y: root.engineReady ? appEngine.rightHandY * cameraViewport.height - 14 : 0
                z: 10
            }
        }
    }

    // ── Inline component: StrumZone ──────────────────────────────────────────
    // Pinch (either hand) inside the zone → rising edge triggers strum.
    // Guitar image is bottom-aligned and fills the entire zone.
    component StrumZone: Item {
        id: sz

        property bool prevLeftPinch:  false
        property bool prevRightPinch: false
        property bool strumCooldown:  false

        Timer {
            id: strumCooldownTimer
            interval: 350   // ms before another strum can fire
            repeat:   false
            onTriggered: sz.strumCooldown = false
        }

        Connections {
            target: root.engineReady ? appEngine : null
            function onHandStateChanged() {
                if (!root.engineReady) return

                const lPinch = appEngine.leftPinch  && root.leftHandInside(sz)
                const rPinch = appEngine.rightPinch && root.rightHandInside(sz)

                // Rising edge + cooldown → fire strum once per pinch
                if (!sz.strumCooldown) {
                    if ((lPinch && !sz.prevLeftPinch) || (rPinch && !sz.prevRightPinch)) {
                        appEngine.triggerGuitarStrum(100)
                        root.strumFlash = true
                        strumFlashTimer.restart()
                        sz.strumCooldown = true
                        strumCooldownTimer.restart()
                    }
                }
                sz.prevLeftPinch  = lPinch
                sz.prevRightPinch = rPinch
            }
        }

        // Zone border — flashes orange on strum, very subtle at rest
        Rectangle {
            anchors.fill: parent
            color: root.strumFlash
                   ? Qt.rgba(0.878, 0.471, 0.149, 0.10)
                   : "transparent"
            border.color: root.strumFlash
                          ? "#E07826"
                          : Qt.rgba(0.878, 0.471, 0.149, 0.25)
            border.width: root.strumFlash ? 2 : 1
            radius: 6

            Behavior on color        { ColorAnimation { duration: 120 } }
            Behavior on border.color { ColorAnimation { duration: 120 } }
        }

        // Guitar image — bottom-aligned, fills full zone width
        Image {
            anchors.fill:        parent
            source:              "qrc:/assets/icons/display_guitar.png"
            fillMode:            Image.PreserveAspectFit
            verticalAlignment:   Image.AlignBottom
            horizontalAlignment: Image.AlignHCenter
            smooth: true
            mipmap: true
            opacity: 0.88
        }
    }

    // ── Inline component: NeckCounter ────────────────────────────────────────
    // Mirrors OctaveGroup + KeyboardZone from KeyboardPage:
    //   thumb up   inside this area → neck goes to lower frets  → green flash
    //   thumb down inside this area → neck goes to higher frets → red flash
    component NeckCounter: Item {
        id: nc

        readonly property var neckLabels: ["1 – 7", "8 – 15", "16 – 24"]

        // Background — flashes green / red
        Rectangle {
            anchors.fill: parent
            color: root.neckFlashDelta > 0 ? Qt.rgba(0.12, 0.72, 0.36, 0.42)
                 : root.neckFlashDelta < 0 ? Qt.rgba(0.9,  0.16, 0.16, 0.42)
                 : Qt.rgba(0.015, 0.018, 0.024, 0.34)
            border.color: root.neckFlashDelta > 0 ? "#2FE174"
                        : root.neckFlashDelta < 0 ? "#FF4D4D"
                        : Qt.rgba(1, 1, 1, 0.32)
            border.width: 1

            Behavior on color        { ColorAnimation { duration: 130 } }
            Behavior on border.color { ColorAnimation { duration: 130 } }
        }

        Column {
            anchors.centerIn: parent
            spacing: 8

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text:           "↑"
                font.family:    figTreeVariable.name
                font.pixelSize: Math.max(22, nc.height * 0.18)
                color: root.neckFlashDelta > 0 ? "#2FE174" : Qt.rgba(1, 1, 1, 0.38)
                Behavior on color { ColorAnimation { duration: 130 } }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text:           "Neck"
                font.family:    figTreeVariable.name
                font.pixelSize: Math.max(16, nc.height * 0.13)
                font.weight:    Font.Light
                font.letterSpacing: 2
                color:          Qt.rgba(1, 1, 1, 0.55)
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text:           nc.neckLabels[root.neckPosition]
                font.family:    figTreeVariable.name
                font.pixelSize: Math.max(24, nc.height * 0.22)
                font.weight:    Font.Medium
                color:          "#EBEDF0"
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text:           "↓"
                font.family:    figTreeVariable.name
                font.pixelSize: Math.max(22, nc.height * 0.18)
                color: root.neckFlashDelta < 0 ? "#FF4D4D" : Qt.rgba(1, 1, 1, 0.38)
                Behavior on color { ColorAnimation { duration: 130 } }
            }
        }
    }

    // ── Inline component: FretboardBars ──────────────────────────────────────
    // fretboard.png as the base image; one of three equal sections is
    // highlighted orange based on neckPosition. No text labels.
    component FretboardBars: Item {
        id: fb

        // Base fretboard image stretched to fill
        Image {
            anchors.fill: parent
            source:       "qrc:/assets/icons/fretboard.png"
            fillMode:     Image.Stretch
            smooth:       true
            mipmap:       true
        }

        // Three highlight sections on top — only the active one is visible
        Repeater {
            model: 3

            delegate: Rectangle {
                required property int index
                x:      index * (fb.width / 3)
                y:      0
                width:  fb.width / 3
                height: fb.height

                color: index === root.neckPosition
                       ? Qt.rgba(0.878, 0.471, 0.149, 0.45)
                       : "transparent"
                border.color: index === root.neckPosition
                              ? "#E07826"
                              : "transparent"
                border.width: 2
                radius: 4

                Behavior on color        { ColorAnimation { duration: 200 } }
                Behavior on border.color { ColorAnimation { duration: 200 } }
            }
        }
    }

    // ── Header ───────────────────────────────────────────────────────────────
    Item {
        id: header
        anchors.top:   parent.top
        anchors.left:  parent.left
        anchors.right: parent.right
        height: 60
        z: 10

        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0.063, 0.071, 0.094, 0.75)
        }

        MouseArea {
            id: backBtn
            width: 48; height: 48
            anchors.left:           parent.left
            anchors.leftMargin:     12
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: true
            cursorShape:  Qt.PointingHandCursor
            onClicked: {
                if (root.engineReady) appEngine.goBack()
                root.back()
            }
            Text {
                anchors.centerIn: parent
                text:           "←"
                font.pixelSize: 24
                color: backBtn.containsMouse ? "#E07A26" : "#949AA5"
                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }

        MouseArea {
            id: settingsBtn
            width: 40; height: 40
            anchors.left:           backBtn.right
            anchors.leftMargin:     8
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: true
            cursorShape:  Qt.PointingHandCursor
            onClicked: settingsPanel.open = !settingsPanel.open

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: settingsBtn.containsMouse || settingsPanel.open
                       ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
                Behavior on color { ColorAnimation { duration: 150 } }

                Image {
                    anchors.centerIn: parent
                    width: 22; height: 22
                    source:            "qrc:/assets/icons/settings.svg"
                    sourceSize.width:  22
                    sourceSize.height: 22
                    fillMode: Image.PreserveAspectFit
                    smooth:   true
                    opacity:  settingsBtn.containsMouse || settingsPanel.open ? 1.0 : 0.85
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }
            }
        }

        Text {
            anchors.centerIn: parent
            text:               "Guitar"
            font.family:        figTreeVariable.name
            font.pixelSize:     20
            font.weight:        Font.Light
            font.letterSpacing: 2
            color: "#E07826"
        }

        TypeSelector {
            anchors.right:          parent.right
            anchors.rightMargin:    20
            anchors.verticalCenter: parent.verticalCenter
            model: ["Acoustic Guitar", "Clean Electric", "Distorted Electric"]
            currentIndex: 0
            onActivated: function(index) {
                if (!root.engineReady) return
                const soundIds = [2, 0, 1]
                appEngine.setGuitarSound(soundIds[index] ?? 2)
            }
        }
    }

    // ── Settings overlay backdrop ────────────────────────────────────────────
    MouseArea {
        anchors.top:    header.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        visible: settingsPanel.open
        onClicked: settingsPanel.open = false
        z: 20
    }

    // ── Settings panel ───────────────────────────────────────────────────────
    SettingsPanel {
        id: settingsPanel
        title: "Guitar Settings"
        font.family: figTreeVariable.name
        anchors.left:       parent.left
        anchors.leftMargin: 20
        anchors.top:        header.bottom
        anchors.topMargin:  12
        z: 30

        Text {
            text: "VOLUME"
            font.family: figTreeVariable.name; font.pixelSize: 11
            font.weight: Font.DemiBold; font.letterSpacing: 1.5; color: "#E07826"
        }
        SettingsSlider { label: "Min Volume"; unit: "%"; value: 10; from: 0; to: 100 }
        SettingsSlider { label: "Max Volume"; unit: "%"; value: 85; from: 0; to: 100 }

        Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.06) }

        Text {
            text: "GESTURE CONTROL"
            font.family: figTreeVariable.name; font.pixelSize: 11
            font.weight: Font.DemiBold; font.letterSpacing: 1.5; color: "#E07826"
        }
        SettingsSlider { label: "Strum Sensitivity"; unit: "%"; value: 60; from: 10; to: 100 }

        Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.06) }

        Text {
            text: "INSTRUMENT"
            font.family: figTreeVariable.name; font.pixelSize: 11
            font.weight: Font.DemiBold; font.letterSpacing: 1.5; color: "#E07826"
        }
        SettingsSlider { label: "Number of Strings"; value: 6; from: 4; to: 6; stepSize: 2 }
        SettingsSlider { label: "Capo Position";     value: 0; from: 0; to: 12; stepSize: 1 }
    }
}
