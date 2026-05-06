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

    // ── Chord wheel state ────────────────────────────────────────────────────
    property bool sharpsEnabled:        false
    property int  selectedChordRoot:    -1   // index into roots array, -1 = no chord
    property int  selectedChordQuality:  0   // index into qualities array

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
                    if (root.engineReady) appEngine.adjustGuitarVoicing(1) // <-- ADD THIS
                    
                    root.neckPosition   = Math.min(2, root.neckPosition + 1)
                    root.neckFlashDelta = +1
                    neckFlashTimer.restart()
                    root.neckCooldown = true
                    neckCooldownTimer.restart()
                } else if (down && !root.prevThumbDown) {
                    if (root.engineReady) appEngine.adjustGuitarVoicing(-1) // <-- ADD THIS
                    
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

                // Left half: chord selector (vertical bar + pop-out semi-circle)
                ChordSelector {
                    x:      0
                    y:      0
                    width:  parent.width  * 0.50
                    height: parent.height
                }

                // Right half: NeckCounter (top) → FretboardBars → StrumZone (bottom)
                Item {
                    id: rightHalf
                    x:      parent.width * 0.50
                    y:      0
                    width:  parent.width  * 0.50
                    height: parent.height

                    // Neck position counter — compact single-line chip at top
                    NeckCounter {
                        id: neckCounter
                        anchors.top:        parent.top
                        anchors.topMargin:  10
                        anchors.left:       parent.left
                        anchors.leftMargin: 8
                        anchors.right:      parent.right
                        anchors.rightMargin: 8
                        height: 50
                    }

                    // Chord name — floats in the gap between fretboard and strum zone
                    Item {
                        visible: root.selectedChordRoot >= 0
                        anchors.top:    fretboardBars.bottom
                        anchors.bottom: strumZone.top
                        anchors.left:   parent.left
                        anchors.right:  parent.right

                        Rectangle {
                            anchors.centerIn: parent
                            width:  chordLabel.implicitWidth + 32
                            height: chordLabel.implicitHeight + 16
                            radius: 10
                            color:        Qt.rgba(0.063, 0.071, 0.094, 0.50)
                            border.color: Qt.rgba(0.878, 0.471, 0.149, 0.40)
                            border.width: 1

                            Text {
                                id: chordLabel
                                anchors.centerIn: parent
                                text: root.selectedChordRoot >= 0
                                      ? (root.sharpsEnabled
                                         ? ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"][root.selectedChordRoot]
                                         : ["C","D","E","F","G","A","B"][root.selectedChordRoot])
                                        + "  "
                                        + ["Maj","Min","7","Maj7","Min7","Sus2","Sus4","Dim","Aug","min7b5"][root.selectedChordQuality]
                                      : ""
                                font.family:    figTreeVariable.name
                                font.pixelSize: 40
                                font.weight:    Font.Bold
                                color:          "#E07826"
                            }
                        }
                    }

                    // Strum zone — anchored to the bottom
                    StrumZone {
                        id: strumZone
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        anchors.bottom: parent.bottom
                        height: parent.height * 0.50
                    }

                    // Fretboard — sits just below the neck counter chip
                    FretboardBars {
                        id: fretboardBars
                        anchors.left:       parent.left
                        anchors.leftMargin: 6
                        anchors.right:      parent.right
                        anchors.rightMargin: 6
                        anchors.top:        neckCounter.bottom
                        anchors.topMargin:  8
                        height: width * (370.0 / 2090.0)
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

        // Drawn guitar — horizontal orientation (rotated 90°), strings + sound hole only
        Canvas {
            id: guitarCanvas
            anchors.fill: parent
            Component.onCompleted: requestPaint()

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                var W  = width
                var H  = height
                var cy = H / 2
                var bodyLeft  = W * 0.02
                var bodyRight = W * 0.98

                // ── Sound hole rosette ─────────────────────────────────────
                var shCX = W * 0.62
                var shR  = Math.min(W * 0.18, H * 0.44)

                ctx.strokeStyle = "rgba(255,255,255,0.04)"
                ctx.lineWidth   = 1
                ctx.beginPath(); ctx.arc(shCX, cy, shR + 10, 0, Math.PI * 2); ctx.stroke()

                ctx.strokeStyle = "rgba(224,120,38,0.18)"
                ctx.lineWidth   = 1
                ctx.beginPath(); ctx.arc(shCX, cy, shR + 7,  0, Math.PI * 2); ctx.stroke()

                ctx.strokeStyle = "rgba(224,120,38,0.40)"
                ctx.lineWidth   = 1.5
                ctx.beginPath(); ctx.arc(shCX, cy, shR + 3,  0, Math.PI * 2); ctx.stroke()

                // Sound hole fill
                ctx.fillStyle = "#040305"
                ctx.beginPath(); ctx.arc(shCX, cy, shR, 0, Math.PI * 2); ctx.fill()

                // Sound hole rim
                ctx.strokeStyle = "rgba(224,120,38,0.60)"
                ctx.lineWidth   = 1.5
                ctx.beginPath(); ctx.arc(shCX, cy, shR, 0, Math.PI * 2); ctx.stroke()

                // ── Saddle (left side — string anchor) ────────────────────
                var saddleX = W * 0.10
                var saddleH = H * 0.38
                ctx.fillStyle = "rgba(205,215,228,0.55)"
                ctx.fillRect(saddleX - 1.5, cy - saddleH * 0.5, 3, saddleH)

                // ── Strings (horizontal, left to right) ───────────────────
                var numStrings = 6
                var strSpacing = H * 0.072
                var s0y        = cy - (numStrings - 1) * strSpacing * 0.5

                for (var s = 0; s < numStrings; s++) {
                    var sy = s0y + s * strSpacing
                    // String 0 = high E (thinnest), string 5 = low E (thickest)
                    ctx.strokeStyle = "rgba(195,208,224,0.65)"
                    ctx.lineWidth   = 1.8 + s * 0.45
                    ctx.beginPath()
                    ctx.moveTo(bodyLeft,  sy)
                    ctx.lineTo(bodyRight, sy)
                    ctx.stroke()
                }
            }
        }
    }

    // ── Inline component: NeckCounter ────────────────────────────────────────
    // Compact single-line chip showing the current fret range.
    // Flashes green (thumb up) or red (thumb down). Detection covers entire rightHalf.
    component NeckCounter: Item {
        id: nc

        readonly property var neckLabels: ["Frets 1 – 7", "Frets 8 – 15", "Frets 16 – 24"]

        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: root.neckFlashDelta > 0 ? Qt.rgba(0.12, 0.72, 0.36, 0.38)
                 : root.neckFlashDelta < 0 ? Qt.rgba(0.9,  0.16, 0.16, 0.38)
                 : Qt.rgba(0.015, 0.018, 0.024, 0.55)
            border.color: root.neckFlashDelta > 0 ? "#2FE174"
                        : root.neckFlashDelta < 0 ? "#FF4D4D"
                        : Qt.rgba(1, 1, 1, 0.18)
            border.width: 1

            Behavior on color        { ColorAnimation { duration: 130 } }
            Behavior on border.color { ColorAnimation { duration: 130 } }
        }

        Row {
            anchors.centerIn: parent
            spacing: 6

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text:           "↑"
                font.family:    figTreeVariable.name
                font.pixelSize: 14
                color: root.neckFlashDelta > 0 ? "#2FE174" : Qt.rgba(1, 1, 1, 0.30)
                Behavior on color { ColorAnimation { duration: 130 } }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text:           nc.neckLabels[root.neckPosition]
                font.family:    figTreeVariable.name
                font.pixelSize: 14
                font.weight:    Font.Medium
                color:          "#EBEDF0"
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text:           "↓"
                font.family:    figTreeVariable.name
                font.pixelSize: 14
                color: root.neckFlashDelta < 0 ? "#FF4D4D" : Qt.rgba(1, 1, 1, 0.30)
                Behavior on color { ColorAnimation { duration: 130 } }
            }
        }
    }

    // ── Inline component: FretboardBars ──────────────────────────────────────
    // Drawn fretboard: equal-temperament fret spacing, 6 strings, position dots.
    // Zones 0/1/2 (frets 1–7 / 8–15 / 16–24) highlighted in orange.
    component FretboardBars: Item {
        id: fb

        Canvas {
            id: fbCanvas
            anchors.fill: parent

            // Reactive mirror — any change triggers repaint
            property int  pNeckPos: root.neckPosition
            property int  pFlash:   root.neckFlashDelta
            onPNeckPosChanged: requestPaint()
            onPFlashChanged:   requestPaint()

            Component.onCompleted: requestPaint()

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                var W = width
                var H = height
                var numFrets   = 24
                var numStrings = 6

                // Equal-temperament fret positions, normalised to [0, W]
                // pos(n) = distance from nut to fret n
                var scale24 = 1.0 - Math.pow(0.5, 24.0 / 12.0)   // ≈ 0.75
                function fretX(n) {
                    return (1.0 - Math.pow(0.5, n / 12.0)) / scale24 * W
                }

                // Zone boundaries (fret indices that delimit each zone)
                var zoneBounds = [[0, 7], [7, 15], [15, 24]]

                // ── Background ───────────────────────────────────────────
                ctx.fillStyle = "rgba(14,16,22,0.92)"
                ctx.fillRect(0, 0, W, H)

                // ── Zone highlights ───────────────────────────────────────
                for (var z = 0; z < 3; z++) {
                    var x1 = fretX(zoneBounds[z][0])
                    var x2 = fretX(zoneBounds[z][1])
                    if (z === root.neckPosition) {
                        ctx.fillStyle = "rgba(224,120,38,0.22)"
                        ctx.fillRect(x1 + 1, 1, x2 - x1 - 2, H - 2)
                        ctx.strokeStyle = "#E07826"
                        ctx.lineWidth   = 1.5
                        ctx.strokeRect(x1 + 1, 1, x2 - x1 - 2, H - 2)
                    }
                }

                // ── Fret lines ────────────────────────────────────────────
                for (var f = 0; f <= numFrets; f++) {
                    var fx = fretX(f)
                    if (f === 0) {
                        // Nut — thick, bright
                        ctx.strokeStyle = "rgba(200,212,230,0.90)"
                        ctx.lineWidth   = 3
                    } else {
                        ctx.strokeStyle = "rgba(110,122,142,0.65)"
                        ctx.lineWidth   = 1
                    }
                    ctx.beginPath()
                    ctx.moveTo(fx, 0)
                    ctx.lineTo(fx, H)
                    ctx.stroke()
                }

                // ── Strings ───────────────────────────────────────────────
                var strPad = H * 0.14
                for (var s = 0; s < numStrings; s++) {
                    var sy = strPad + (s / (numStrings - 1)) * (H - 2 * strPad)
                    ctx.strokeStyle = "rgba(195,208,224,0.50)"
                    ctx.lineWidth   = 0.7 + s * 0.22
                    ctx.beginPath()
                    ctx.moveTo(0, sy)
                    ctx.lineTo(W, sy)
                    ctx.stroke()
                }

                // ── Outer border ──────────────────────────────────────────
                ctx.strokeStyle = "rgba(255,255,255,0.08)"
                ctx.lineWidth   = 1
                ctx.strokeRect(0.5, 0.5, W - 1, H - 1)
            }
        }
    }

    // ── Inline component: ChordWheel ─────────────────────────────────────────
    // Two-level selection:
    //   1. Pinch outer ring  → select root (C, D# …)
    //   2. Pinch inner ring  → select quality (Maj, Min, 7 …)
    //   3. Pinch centre      → clear selection (no chord)
    // Sharps mode (toggle in settings) swaps 7 natural roots for all 12.
    // ── Inline component: ChordSelector ──────────────────────────────────────
    // Left side: vertical bar of root notes (A–G or chromatic with sharps).
    // Pinch a root → semi-circle fans out to the RIGHT showing chord qualities.
    // Pinch a quality → chord locked in. Pinch root again → deselect.
    component ChordSelector: Item {
        id: cs

        // ── Data ──────────────────────────────────────────────────────────
        readonly property var naturalRoots: ["C","D","E","F","G","A","B"]
        readonly property var sharpRoots:   ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"]
        readonly property var roots:     root.sharpsEnabled ? sharpRoots : naturalRoots
        readonly property int numRoots:  roots.length
        readonly property var qualities: ["Maj","Min","7","Maj7","Min7","Sus2","Sus4","Dim","Aug","min7b5"]
        readonly property int numQ:      10

        // ── Geometry ──────────────────────────────────────────────────────
        // Bar: left edge, full height, fixed width
        readonly property real barW:    Math.max(44, width * 0.24)
        // Semi-circle: always centred vertically in the component
        readonly property real semiCY:  height / 2
        readonly property real semiR:   Math.min(width - barW, height * 0.52)
        readonly property real innerR:  semiR * 0.38    // larger dead zone near centre
        readonly property real rowH:    height / numRoots

        // ── Interaction state ─────────────────────────────────────────────
        property int  hovRoot:    -1   // index or -1
        property int  hovQuality: -1
        property bool prevLPinch: false
        property bool prevRPinch: false

        // Returns { zone: "bar"|"semi"|"none",  index: N }
        function hitTest(nx, ny) {
            var pt = cs.mapFromItem(cameraViewport,
                                    nx * cameraViewport.width,
                                    ny * cameraViewport.height)
            // Bar zone
            if (pt.x >= 0 && pt.x < cs.barW && pt.y >= 0 && pt.y < cs.height) {
                var ri = Math.min(cs.numRoots - 1,
                                  Math.floor(pt.y / cs.height * cs.numRoots))
                return { zone: "bar", index: ri }
            }
            // Semi-circle zone (only when a root is selected)
            if (root.selectedChordRoot >= 0) {
                var cy = cs.semiCY
                var dx = pt.x - cs.barW
                var dy = pt.y - cy
                var dist = Math.sqrt(dx * dx + dy * dy)
                if (dx >= 0 && dist >= cs.innerR && dist <= cs.semiR) {
                    var angle = Math.atan2(dy, dx)   // −π/2 … +π/2 for right half
                    if (angle >= -Math.PI / 2 && angle <= Math.PI / 2) {
                        var norm = (angle + Math.PI / 2) / Math.PI
                        var qi   = Math.min(cs.numQ - 1, Math.floor(norm * cs.numQ))
                        return { zone: "semi", index: qi }
                    }
                }
            }
            return { zone: "none", index: -1 }
        }

        // Connections — hover tracking + pinch rising-edge selection
        Connections {
            target: root.engineReady ? appEngine : null

            function onHandStateChanged() {
                if (!root.engineReady) return

                var hit = { zone: "none", index: -1 }
                if (appEngine.leftHandVisible)
                    hit = cs.hitTest(appEngine.leftHandX, appEngine.leftHandY)
                if (hit.zone === "none" && appEngine.rightHandVisible)
                    hit = cs.hitTest(appEngine.rightHandX, appEngine.rightHandY)

                cs.hovRoot    = hit.zone === "bar"  ? hit.index : -1
                cs.hovQuality = hit.zone === "semi" ? hit.index : -1

                var active = hit.zone !== "none"
                var lp = appEngine.leftPinch  && active
                var rp = appEngine.rightPinch && active

                // --- THIS IS THE EXACT BLOCK YOU REPLACE ---
                if ((lp && !cs.prevLPinch) || (rp && !cs.prevRPinch)) {
                    if (hit.zone === "bar") {
                        // Toggle root selection
                        if (root.selectedChordRoot === hit.index) {
                            root.selectedChordRoot = -1
                            if (root.engineReady) appEngine.setGuitarChord(-1, 0) // <-- ADD THIS
                        } else {
                            root.selectedChordRoot    = hit.index
                            root.selectedChordQuality = 0
                            if (root.engineReady) appEngine.setGuitarChord(hit.index, 0) // <-- ADD THIS
                        }
                    } else if (hit.zone === "semi") {
                        root.selectedChordQuality = hit.index
                        if (root.engineReady) appEngine.setGuitarChord(root.selectedChordRoot, hit.index) // <-- ADD THIS
                    }
                }
                // -------------------------------------------

                cs.prevLPinch = lp
                cs.prevRPinch = rp
            }
        }

        // ── Canvas ────────────────────────────────────────────────────────
        Canvas {
            id: csCanvas
            anchors.fill: parent

            property int    pSelRoot:  root.selectedChordRoot
            property int    pSelQual:  root.selectedChordQuality
            property int    pHovRoot:  cs.hovRoot
            property int    pHovQual:  cs.hovQuality
            property int    pNumRoots: cs.numRoots
            property bool   pSharps:   root.sharpsEnabled
            property string pFont:     figTreeVariable.name

            onPSelRootChanged:  requestPaint()
            onPSelQualChanged:  requestPaint()
            onPHovRootChanged:  requestPaint()
            onPHovQualChanged:  requestPaint()
            onPNumRootsChanged: requestPaint()
            onPSharpsChanged:   requestPaint()
            onPFontChanged:     requestPaint()

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                var barW   = cs.barW
                var semiR  = cs.semiR
                var innerR = cs.innerR
                var rowH   = cs.rowH
                var n      = cs.numRoots
                var nQ     = cs.numQ
                var sR     = root.selectedChordRoot
                var sQ     = root.selectedChordQuality
                var hR     = cs.hovRoot
                var hQ     = cs.hovQuality

                // ── Semi-circle (draw before bar so bar appears on top) ───
                if (sR >= 0) {
                    var scY = cs.semiCY

                    // Faint backdrop
                    ctx.beginPath()
                    ctx.moveTo(barW, scY)
                    ctx.arc(barW, scY, semiR, -Math.PI / 2, Math.PI / 2)
                    ctx.closePath()
                    ctx.fillStyle = "rgba(8,11,18,0.65)"
                    ctx.fill()

                    // Quality segments (annular sectors, right half)
                    for (var q = 0; q < nQ; q++) {
                        var startA = -Math.PI / 2 + (q / nQ) * Math.PI
                        var endA   = -Math.PI / 2 + ((q + 1) / nQ) * Math.PI

                        var isSelQ = (q === sQ)
                        var isHovQ = (q === hQ)

                        ctx.beginPath()
                        ctx.arc(barW, scY, semiR - 1, startA, endA)
                        ctx.arc(barW, scY, innerR + 1, endA, startA, true)
                        ctx.closePath()

                        ctx.fillStyle = isSelQ ? "rgba(224,120,38,0.82)"
                                      : isHovQ ? "rgba(224,120,38,0.28)"
                                      : "rgba(20,24,36,0.82)"
                        ctx.fill()
                        ctx.strokeStyle = (isSelQ || isHovQ) ? "#E07826" : "rgba(255,255,255,0.09)"
                        ctx.lineWidth   = isSelQ ? 2 : 1
                        ctx.stroke()

                        // Quality label
                        var midA   = (startA + endA) / 2
                        var labelR = (semiR + innerR) / 2
                        var lx = barW + Math.cos(midA) * labelR
                        var ly = scY  + Math.sin(midA) * labelR
                        var qfs = Math.max(10, Math.min(16, (semiR - innerR) * 0.22))
                        ctx.font         = (isSelQ ? "700 " : "500 ") + qfs + "px '" + csCanvas.pFont + "'"
                        ctx.textAlign    = "center"
                        ctx.textBaseline = "middle"
                        ctx.fillStyle    = isSelQ ? "#FFFFFF" : isHovQ ? "#FFD4A0" : "rgba(200,212,225,0.85)"
                        ctx.fillText(cs.qualities[q], lx, ly)
                    }

                    // Donut hole
                    ctx.beginPath()
                    ctx.arc(barW, scY, innerR, -Math.PI / 2, Math.PI / 2)
                    ctx.arc(barW, scY, 0, Math.PI / 2, -Math.PI / 2, true)
                    ctx.closePath()
                    ctx.fillStyle = "rgba(10,13,20,0.92)"
                    ctx.fill()

                    // Connector line from selected bar row to semi-circle
                    ctx.strokeStyle = "#E07826"
                    ctx.lineWidth   = 1.5
                    ctx.setLineDash([4, 4])
                    ctx.beginPath()
                    ctx.moveTo(barW, scY)
                    ctx.lineTo(barW + innerR, scY)
                    ctx.stroke()
                    ctx.setLineDash([])
                }

                // ── Vertical bar ─────────────────────────────────────────
                for (var i = 0; i < n; i++) {
                    var y      = i * rowH
                    var isSel  = (i === sR)
                    var isHov  = (i === hR)
                    var pad    = 4

                    // Row background
                    ctx.fillStyle = isSel ? "rgba(224,120,38,0.85)"
                                  : isHov ? "rgba(224,120,38,0.32)"
                                  : "rgba(16,20,30,0.80)"
                    ctx.beginPath()
                    ctx.rect(pad, y + pad, barW - pad * 2, rowH - pad * 2)
                    ctx.fill()

                    // Row border
                    ctx.strokeStyle = (isSel || isHov) ? "#E07826" : "rgba(255,255,255,0.10)"
                    ctx.lineWidth   = isSel ? 2 : 1
                    ctx.stroke()

                    // Root label — big, bold
                    var fs = Math.max(14, Math.min(30, rowH * 0.48))
                    ctx.font         = "700 " + fs + "px '" + csCanvas.pFont + "'"
                    ctx.textAlign    = "center"
                    ctx.textBaseline = "middle"
                    ctx.fillStyle    = isSel ? "#FFFFFF" : isHov ? "#FFD4A0" : "rgba(235,237,240,0.90)"
                    ctx.fillText(cs.roots[i], barW / 2, y + rowH / 2)
                }
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
            text: "CHORD WHEEL"
            font.family: figTreeVariable.name; font.pixelSize: 11
            font.weight: Font.DemiBold; font.letterSpacing: 1.5; color: "#E07826"
        }

        SettingsToggle {
            label:   "Show Sharps / Flats"
            checked: root.sharpsEnabled
            onCheckedChanged: root.sharpsEnabled = checked
        }

        Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.06) }

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
