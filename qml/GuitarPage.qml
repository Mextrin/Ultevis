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

    // ── Camera feed (fills entire area below header, aspect-fit) ─────────────
    Image {
        id: cameraFeed
        anchors.top:    header.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
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

    // ── Camera viewport — tracks the actual painted region ───────────────────
    Item {
        id: cameraViewport
        readonly property real contentWidth:  cameraFeed.paintedWidth  > 0 ? cameraFeed.paintedWidth  : cameraFeed.width
        readonly property real contentHeight: cameraFeed.paintedHeight > 0 ? cameraFeed.paintedHeight : cameraFeed.height
        x:      cameraFeed.x + (cameraFeed.width  - contentWidth)  / 2
        y:      cameraFeed.y + (cameraFeed.height - contentHeight) / 2
        width:  contentWidth
        height: contentHeight
        z: 5

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
                        anchors.rightMargin: 20
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 30
                        height: parent.height * 0.45
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

    // ── Inline component: StrumZone ──────────────────────────────────────────
    // Pinch (either hand) inside the zone → rising edge triggers strum.
    // Guitar image is bottom-aligned and fills the entire zone.
    component StrumZone: Item {
        id: sz

        property bool prevLeftPinch:  false
        property bool prevRightPinch: false
        property bool strumCooldown:  false
        property bool canStrum:       true

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

                const rightHandInZone = root.rightHandInside(sz)

                if (!rightHandInZone) {
                    sz.canStrum = true
                }

                const rPinch = appEngine.rightPinch && rightHandInZone

                // Rising edge + cooldown + canStrum state → fire strum
                if (!sz.strumCooldown && sz.canStrum) {
                    if (rPinch && !sz.prevRightPinch) {
                        appEngine.triggerGuitarStrum(100)
                        root.strumFlash = true
                        strumFlashTimer.restart()
                        sz.strumCooldown = true
                        strumCooldownTimer.restart()
                        sz.canStrum = false // Strum used, must exit and re-enter
                    }
                }
                sz.prevRightPinch = rPinch

                // Allow left-hand pinch to work as before, if needed.
                const lPinch = appEngine.leftPinch && root.leftHandInside(sz)
                if (!sz.strumCooldown) {
                    if (lPinch && !sz.prevLeftPinch) {
                        appEngine.triggerGuitarStrum(100)
                        root.strumFlash = true
                        strumFlashTimer.restart()
                        sz.strumCooldown = true
                        strumCooldownTimer.restart()
                    }
                }
                sz.prevLeftPinch = lPinch
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
                var shR  = Math.min(W * 0.26, H * 0.44)

                ctx.strokeStyle = "rgba(255,255,255,0.04)"
                ctx.lineWidth   = 1
                ctx.beginPath(); ctx.arc(shCX, cy, shR + 14, 0, Math.PI * 2); ctx.stroke()

                ctx.strokeStyle = "rgba(224,120,38,0.18)"
                ctx.lineWidth   = 1
                ctx.beginPath(); ctx.arc(shCX, cy, shR + 10, 0, Math.PI * 2); ctx.stroke()

                ctx.strokeStyle = "rgba(224,120,38,0.40)"
                ctx.lineWidth   = 1.5
                ctx.beginPath(); ctx.arc(shCX, cy, shR + 4,  0, Math.PI * 2); ctx.stroke()

                // Sound hole fill
                ctx.fillStyle = "#040305"
                ctx.beginPath(); ctx.arc(shCX, cy, shR, 0, Math.PI * 2); ctx.fill()

                // Sound hole rim
                ctx.strokeStyle = "rgba(224,120,38,0.60)"
                ctx.lineWidth   = 1.5
                ctx.beginPath(); ctx.arc(shCX, cy, shR, 0, Math.PI * 2); ctx.stroke()

                // ── Saddle (left side — string anchor) ────────────────────
                var saddleX = W * 0.10
                var saddleH = H * 0.60
                ctx.fillStyle = "rgba(205,215,228,0.55)"
                ctx.fillRect(saddleX - 1.5, cy - saddleH * 0.5, 3, saddleH)

                // ── Strings (horizontal, left to right) ───────────────────
                var numStrings = 6
                var strSpacing = H * 0.11
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

    // ── Inline component: ChordSelector ──────────────────────────────────────
    // Narrow root column sits toward the centre of the left half (not at the
    // far-left edge).  Pinch + hold on a root locks it and a left-facing
    // semicircle fans open to the left — staying entirely within the left half
    // and never crossing the centre line.  Moving the hand around the arc
    // sweeps through qualities; releasing commits the chord.
    component ChordSelector: Item {
        id: cs

        // ── Data ──────────────────────────────────────────────────────────
        readonly property var naturalRoots: ["C","D","E","F","G","A","B"]
        readonly property var sharpRoots:   ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"]
        readonly property var roots:     root.sharpsEnabled ? sharpRoots : naturalRoots
        readonly property int numRoots:  roots.length
        readonly property var qualities: ["Maj","Min","7","Maj7","Min7","Sus2","Sus4"]
        readonly property int numQ:      7

        // ── Geometry ──────────────────────────────────────────────────────
        // Column hugs the left edge; semicircle fans RIGHT from the column's
        // right edge and is capped so it never crosses the half-line (cs.width).
        readonly property real colX:   width * 0.08   // left edge of root column
        readonly property real colW:   width * 0.18   // column width
        readonly property real semiCX: colX + colW    // semicircle centre X (right edge of column)
        readonly property real semiCY: height * 0.50  // semicircle always vertically centred
        readonly property real rowH:   height / numRoots
        // semiR: fills as much of the remaining left half as possible
        readonly property real semiR:  Math.min((width - semiCX) * 0.92, height * 0.46)
        readonly property real innerR: semiR * 0.28

        // ── Interaction state ─────────────────────────────────────────────
        property int  hovRoot:     -1
        property int  dragQuality:  0
        property bool dragging:    false
        property bool prevLPinch:  false

        // ── Helpers ───────────────────────────────────────────────────────
        function localPt(nx, ny) {
            return cs.mapFromItem(cameraViewport,
                                  nx * cameraViewport.width,
                                  ny * cameraViewport.height)
        }

        function rootAtY(py) {
            return Math.max(0, Math.min(cs.numRoots - 1,
                            Math.floor(py / cs.height * cs.numRoots)))
        }

        // Map an angle in the RIGHT semicircle to a quality index.
        // Right semicircle spans -π/2 (top) to +π/2 (bottom) via 0 (right).
        // atan2 returns values directly in that range when dx >= 0.
        function qualityFromAngle(dx, dy) {
            var angle = Math.atan2(dy, dx)                 // in [-π/2, +π/2] for right half
            var norm  = (angle + Math.PI / 2) / Math.PI   // 0 (top) → 1 (bottom)
            return Math.max(0, Math.min(cs.numQ - 1, Math.floor(norm * cs.numQ)))
        }

        // ── Input ─────────────────────────────────────────────────────────
        Connections {
            target: root.engineReady ? appEngine : null

            function onHandStateChanged() {
                if (!root.engineReady || !appEngine.leftHandVisible) {
                    if (cs.dragging) {
                        root.selectedChordQuality = cs.dragQuality
                        cs.dragging = false
                    }
                    cs.hovRoot    = -1
                    cs.prevLPinch = false
                    return
                }

                var pt = cs.localPt(appEngine.leftHandX, appEngine.leftHandY)
                var lp = appEngine.leftPinch

                if (cs.dragging) {
                    if (lp) {
                        // While pinching: update quality from position in semicircle
                        if (root.selectedChordRoot >= 0) {
                            var dx   = pt.x - cs.semiCX
                            var dy   = pt.y - cs.semiCY
                            var dist = Math.sqrt(dx * dx + dy * dy)
                            // Only update when hand is inside the right-facing arc
                            if (dx >= 0 && dist >= cs.innerR && dist <= cs.semiR)
                                cs.dragQuality = cs.qualityFromAngle(dx, dy)
                        }
                    } else {
                        // Pinch released — commit
                        root.selectedChordQuality = cs.dragQuality
                        cs.dragging = false
                    }
                } else {
                    // Hover on root column
                    var inCol = pt.x >= cs.colX && pt.x <= cs.colX + cs.colW &&
                                pt.y >= 0 && pt.y <= cs.height
                    cs.hovRoot = inCol ? cs.rootAtY(pt.y) : -1

                    // Rising-edge pinch on column → lock root, open semicircle
                    if (lp && !cs.prevLPinch && inCol) {
                        var ri = cs.rootAtY(pt.y)
                        if (root.selectedChordRoot === ri) {
                            root.selectedChordRoot = -1   // tap same root = deselect
                        } else {
                            root.selectedChordRoot = ri
                            cs.dragQuality = root.selectedChordQuality
                            cs.dragging    = true
                        }
                    }
                }

                cs.prevLPinch = lp
            }
        }

        // ── Canvas ────────────────────────────────────────────────────────
        Canvas {
            id: csCanvas
            anchors.fill: parent

            property int    pSelRoot:   root.selectedChordRoot
            property int    pSelQual:   root.selectedChordQuality
            property int    pHovRoot:   cs.hovRoot
            property int    pDragQual:  cs.dragQuality
            property bool   pDragging:  cs.dragging
            property int    pNumRoots:  cs.numRoots
            property bool   pSharps:    root.sharpsEnabled
            property string pFont:      figTreeVariable.name

            onPSelRootChanged:  requestPaint()
            onPSelQualChanged:  requestPaint()
            onPHovRootChanged:  requestPaint()
            onPDragQualChanged: requestPaint()
            onPDraggingChanged: requestPaint()
            onPNumRootsChanged: requestPaint()
            onPSharpsChanged:   requestPaint()
            onPFontChanged:     requestPaint()

            onPaint: {
                var ctx    = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                var colX   = cs.colX
                var colW   = cs.colW
                var rowH   = cs.rowH
                var semiR  = cs.semiR
                var innerR = cs.innerR
                var n      = cs.numRoots
                var nQ     = cs.numQ
                var sR     = root.selectedChordRoot
                var sQ     = root.selectedChordQuality
                var hR     = cs.hovRoot
                var dQ     = cs.dragQuality
                var isDrag = cs.dragging
                var pad    = 3

                // ── Right-facing semicircle (drawn behind column) ─────────
                var scX = cs.semiCX   // static: right edge of column
                var scY = cs.semiCY   // static: vertical centre

                if (sR >= 0) {
                    // Backdrop: right-facing pie slice
                    ctx.beginPath()
                    ctx.moveTo(scX, scY)
                    ctx.arc(scX, scY, semiR, -Math.PI / 2, Math.PI / 2, false)
                    ctx.closePath()
                    ctx.fillStyle = "rgba(8,10,16,0.72)"
                    ctx.fill()

                    // Quality segments — annular sectors from -π/2 (top) to +π/2 (bottom)
                    for (var q = 0; q < nQ; q++) {
                        var startA  = -Math.PI / 2 + (q / nQ) * Math.PI
                        var endA    = -Math.PI / 2 + ((q + 1) / nQ) * Math.PI
                        var isSelQ  = (q === sQ) && !isDrag
                        var isDragQ = isDrag && (q === dQ)

                        ctx.beginPath()
                        ctx.arc(scX, scY, semiR - 1, startA, endA)
                        ctx.arc(scX, scY, innerR + 1, endA, startA, true)
                        ctx.closePath()

                        ctx.fillStyle   = isDragQ ? "rgba(224,120,38,0.88)"
                                        : isSelQ  ? "rgba(224,120,38,0.60)"
                                        : "rgba(20,24,34,0.80)"
                        ctx.fill()
                        ctx.strokeStyle = (isDragQ || isSelQ) ? "#E07826" : "rgba(255,255,255,0.08)"
                        ctx.lineWidth   = (isDragQ || isSelQ) ? 2 : 1
                        ctx.stroke()

                        // Quality label at arc midpoint
                        var midA   = (startA + endA) / 2
                        var labelR = (semiR + innerR) / 2
                        var lx = scX + Math.cos(midA) * labelR
                        var ly = scY + Math.sin(midA) * labelR
                        var qfs = Math.max(9, Math.min(15, (semiR - innerR) * 0.24))
                        ctx.font         = ((isDragQ || isSelQ) ? "700 " : "500 ") + qfs + "px '" + csCanvas.pFont + "'"
                        ctx.textAlign    = "center"
                        ctx.textBaseline = "middle"
                        ctx.fillStyle    = (isDragQ || isSelQ) ? "#FFFFFF" : "rgba(190,202,218,0.80)"
                        ctx.fillText(cs.qualities[q], lx, ly)
                    }

                    // Donut hole — right half only, so it doesn't bleed into the column
                    ctx.beginPath()
                    ctx.moveTo(scX, scY)
                    ctx.arc(scX, scY, innerR, -Math.PI / 2, Math.PI / 2, false)
                    ctx.closePath()
                    ctx.fillStyle = "rgba(10,12,18,0.92)"
                    ctx.fill()

                    // Dashed connector rightward from column edge to inner radius
                    ctx.strokeStyle = "#E07826"
                    ctx.lineWidth   = 1.5
                    ctx.setLineDash([4, 4])
                    ctx.beginPath()
                    ctx.moveTo(scX, scY)
                    ctx.lineTo(scX + innerR, scY)
                    ctx.stroke()
                    ctx.setLineDash([])
                }

                // ── Root column (drawn on top of semicircle) ──────────────
                for (var i = 0; i < n; i++) {
                    var ry    = i * rowH
                    var isSel = (i === sR)
                    var isHov = (i === hR)

                    ctx.fillStyle = isSel ? "rgba(224,120,38,0.88)"
                                  : isHov ? "rgba(224,120,38,0.32)"
                                  : "rgba(16,20,30,0.82)"
                    ctx.beginPath()
                    ctx.rect(colX + pad, ry + pad, colW - pad * 2, rowH - pad * 2)
                    ctx.fill()

                    ctx.strokeStyle = (isSel || isHov) ? "#E07826" : "rgba(255,255,255,0.10)"
                    ctx.lineWidth   = isSel ? 2 : 1
                    ctx.stroke()

                    var rfs = Math.max(12, Math.min(28, rowH * 0.44))
                    ctx.font         = "700 " + rfs + "px '" + csCanvas.pFont + "'"
                    ctx.textAlign    = "center"
                    ctx.textBaseline = "middle"
                    ctx.fillStyle    = isSel ? "#FFFFFF" : isHov ? "#FFD4A0" : "rgba(235,237,240,0.90)"
                    ctx.fillText(cs.roots[i], colX + colW / 2, ry + rowH / 2)
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
            text: "MIDI OUTPUT"
            font.family: figTreeVariable.name; font.pixelSize: 11
            font.weight: Font.DemiBold; font.letterSpacing: 1.5; color: "#E07826"
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
            from: 0; to: 100; stepSize: 1
            width: parent.width
            value: typeof appEngine !== "undefined" && appEngine.viewState ? (appEngine.viewState.masterVolume * 100) : 100
            onValueChanged: {
                if (typeof appEngine !== "undefined")
                    appEngine.setMasterVolume(value / 100.0)
            }
        }

        Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.06) }
       
    }
}