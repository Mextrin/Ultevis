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
        if (!item || guitarStage.width <= 0 || guitarStage.height <= 0)
            return false
        const pt = item.mapFromItem(guitarStage,
                                    x * guitarStage.width,
                                    y * guitarStage.height)
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

    // ── Camera feed — fills below header, full width ─────────────────────────
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

    // ── Guitar stage — aligned to painted camera content ─────────────────────
    Item {
        id: guitarStage
        z: 5

        readonly property real contentWidth:  cameraFeed.paintedWidth  > 0 ? cameraFeed.paintedWidth  : cameraFeed.width
        readonly property real contentHeight: cameraFeed.paintedHeight > 0 ? cameraFeed.paintedHeight : cameraFeed.height

        x: cameraFeed.x + (cameraFeed.width  - contentWidth)  / 2
        y: cameraFeed.y + (cameraFeed.height - contentHeight) / 2
        width:  contentWidth
        height: contentHeight

        // Subtle dark tint
        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0.01, 0.012, 0.018, 0.16)
        }

        // ── Guitar overlay ────────────────────────────────────────────────────
        Item {
            id: guitarOverlay
            anchors.fill: parent

            // Left half: chord selector (full circle)
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
                    anchors.top:         parent.top
                    anchors.topMargin:   10
                    anchors.left:        parent.left
                    anchors.leftMargin:  8
                    anchors.right:       parent.right
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
                                    + ["Maj","Min","7","M7","m7","S2","S4","Dim","Aug","m7♭5"][root.selectedChordQuality]
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
                    anchors.left:        parent.left
                    anchors.leftMargin:  6
                    anchors.right:       parent.right
                    anchors.rightMargin: 6
                    anchors.top:         neckCounter.bottom
                    anchors.topMargin:   8
                    height: width * (370.0 / 2090.0)
                }
            }
        }

        // ── Hand position dots ────────────────────────────────────────────────
        Rectangle {
            visible: root.engineReady && appEngine.leftHandVisible
            width: 28; height: 28; radius: 14
            color:        root.engineReady && appEngine.leftPinch ? "#e07826" : "#deab84"
            border.color: "#FFFFFF"
            border.width: 2
            x: root.engineReady ? appEngine.leftHandX  * guitarStage.width  - 14 : 0
            y: root.engineReady ? appEngine.leftHandY  * guitarStage.height - 14 : 0
            z: 10
        }

        Rectangle {
            visible: root.engineReady && appEngine.rightHandVisible
            width: 28; height: 28; radius: 14
            color:        root.engineReady && appEngine.rightPinch ? "#e07826" : "#deab84"
            border.color: "#FFFFFF"
            border.width: 2
            x: root.engineReady ? appEngine.rightHandX * guitarStage.width  - 14 : 0
            y: root.engineReady ? appEngine.rightHandY * guitarStage.height - 14 : 0
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

    // ── Inline component: ChordSelector (full circle) ────────────────────────
    // Left semicircle  → root notes (C–B), always visible.
    // Right semicircle → chord qualities, revealed once a root is selected.
    // Centre hole      → shows selected chord name.
    component ChordSelector: Item {
        id: cs

        // ── Data ──────────────────────────────────────────────────────────
        readonly property var naturalRoots:  ["C","D","E","F","G","A","B"]
        readonly property var sharpRoots:    ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"]
        readonly property var roots:         root.sharpsEnabled ? sharpRoots : naturalRoots
        readonly property int numRoots:      roots.length
        readonly property var qualityShort:  ["Maj","Min","7","M7","m7","S2","S4","Dim","Aug","m7♭5"]
        readonly property int numQ:          10

        // ── Geometry ──────────────────────────────────────────────────────
        readonly property real cx:     width  / 2
        readonly property real cy:     height / 2
        readonly property real outerR: Math.min(width * 0.49, height * 0.49)
        readonly property real innerR: outerR * 0.22

        // ── Fade state for right semicircle ───────────────────────────────
        property real qualityOpacity: root.selectedChordRoot >= 0 ? 1.0 : 0.0
        Behavior on qualityOpacity {
            NumberAnimation { duration: 280; easing.type: Easing.OutCubic }
        }

        // ── Interaction state ─────────────────────────────────────────────
        property int  hovRoot:    -1
        property int  hovQuality: -1
        property bool prevLPinch: false
        property bool prevRPinch: false

        // Returns { side: "root"|"quality"|"none", index }
        function hitTest(nx, ny) {
            var pt   = cs.mapFromItem(guitarStage,
                                      nx * guitarStage.width,
                                      ny * guitarStage.height)
            var dx   = pt.x - cs.cx
            var dy   = pt.y - cs.cy
            var dist = Math.sqrt(dx * dx + dy * dy)

            if (dist < cs.innerR || dist > cs.outerR)
                return { side: "none", index: -1 }

            var angle = Math.atan2(dy, dx)   // –π … +π

            if (dx < 0) {
                // Left semicircle — roots
                // Arc runs π/2 → 3π/2 clockwise through π (left).
                // Normalise atan2 result into [π/2, 3π/2].
                var norm = angle < 0 ? angle + 2 * Math.PI : angle
                if (norm < Math.PI / 2 || norm > 3 * Math.PI / 2)
                    return { side: "none", index: -1 }
                var ri = Math.min(cs.numRoots - 1,
                                  Math.floor((norm - Math.PI / 2) / Math.PI * cs.numRoots))
                return { side: "root", index: ri }
            } else {
                // Right semicircle — qualities (only interactive when root is selected)
                if (root.selectedChordRoot < 0)
                    return { side: "none", index: -1 }
                if (angle < -Math.PI / 2 || angle > Math.PI / 2)
                    return { side: "none", index: -1 }
                var qi = Math.min(cs.numQ - 1,
                                  Math.floor((angle + Math.PI / 2) / Math.PI * cs.numQ))
                return { side: "quality", index: qi }
            }
        }

        // ── Connections ───────────────────────────────────────────────────
        Connections {
            target: root.engineReady ? appEngine : null

            function onHandStateChanged() {
                if (!root.engineReady) return

                var hit = { side: "none", index: -1 }
                if (appEngine.leftHandVisible)
                    hit = cs.hitTest(appEngine.leftHandX, appEngine.leftHandY)
                if (hit.side === "none" && appEngine.rightHandVisible)
                    hit = cs.hitTest(appEngine.rightHandX, appEngine.rightHandY)

                cs.hovRoot    = hit.side === "root"    ? hit.index : -1
                cs.hovQuality = hit.side === "quality" ? hit.index : -1

                var active = hit.side !== "none"
                var lp = appEngine.leftPinch  && active
                var rp = appEngine.rightPinch && active

                if ((lp && !cs.prevLPinch) || (rp && !cs.prevRPinch)) {
                    if (hit.side === "root") {
                        // Toggle root; reset quality to 0
                        root.selectedChordRoot    = (root.selectedChordRoot === hit.index)
                                                    ? -1 : hit.index
                        root.selectedChordQuality = 0
                    } else if (hit.side === "quality") {
                        root.selectedChordQuality = hit.index
                    }
                }

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
            property real   pQualOp:   cs.qualityOpacity

            onPSelRootChanged:  requestPaint()
            onPSelQualChanged:  requestPaint()
            onPHovRootChanged:  requestPaint()
            onPHovQualChanged:  requestPaint()
            onPNumRootsChanged: requestPaint()
            onPSharpsChanged:   requestPaint()
            onPFontChanged:     requestPaint()
            onPQualOpChanged:   requestPaint()

            onPaint: {
                var ctx    = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                var ccx    = cs.cx
                var ccy    = cs.cy
                var oR     = cs.outerR
                var iR     = cs.innerR
                var nR     = cs.numRoots
                var nQ     = cs.numQ
                var sR     = root.selectedChordRoot
                var sQ     = root.selectedChordQuality
                var hR     = cs.hovRoot
                var hQ     = cs.hovQuality
                var fnt      = csCanvas.pFont
                var qualOp   = csCanvas.pQualOp    // 0.0 → 1.0 animated
                var rootActive = sR >= 0

                // ── Left semicircle: roots ────────────────────────────────
                // Sectors span π/2 → 3π/2 clockwise (through left / π)
                for (var r = 0; r < nR; r++) {
                    var rA0 = Math.PI / 2 + (r / nR) * Math.PI
                    var rA1 = Math.PI / 2 + ((r + 1) / nR) * Math.PI
                    var isSel = (r === sR)
                    var isHov = (r === hR)

                    ctx.beginPath()
                    ctx.moveTo(ccx, ccy)
                    ctx.arc(ccx, ccy, oR, rA0, rA1)
                    ctx.closePath()

                    ctx.fillStyle = isSel ? "rgba(224,120,38,0.85)"
                                  : isHov ? "rgba(224,120,38,0.30)"
                                  :         "rgba(16,20,32,0.78)"
                    ctx.fill()
                    ctx.strokeStyle = (isSel || isHov) ? "#E07826" : "rgba(255,255,255,0.07)"
                    ctx.lineWidth   = isSel ? 1.5 : 1
                    ctx.stroke()

                    // Label
                    var rMid  = (rA0 + rA1) / 2
                    var rLblR = (oR + iR) / 2
                    var rlx   = ccx + Math.cos(rMid) * rLblR
                    var rly   = ccy + Math.sin(rMid) * rLblR
                    var rfs   = Math.max(11, Math.min(20, oR * 0.14))
                    ctx.font         = (isSel ? "700 " : "600 ") + rfs + "px '" + fnt + "'"
                    ctx.textAlign    = "center"
                    ctx.textBaseline = "middle"
                    ctx.fillStyle    = isSel ? "#FFFFFF" : isHov ? "#FFD4A0" : "rgba(220,228,240,0.90)"
                    ctx.fillText(cs.roots[r], rlx, rly)
                }

                // ── Right semicircle: qualities ───────────────────────────
                // Sectors span –π/2 → π/2 clockwise (through right / 0)
                for (var q = 0; q < nQ; q++) {
                    var qA0    = -Math.PI / 2 + (q / nQ) * Math.PI
                    var qA1    = -Math.PI / 2 + ((q + 1) / nQ) * Math.PI
                    var isSelQ = (q === sQ && rootActive)
                    var isHovQ = (q === hQ && rootActive)

                    ctx.beginPath()
                    ctx.moveTo(ccx, ccy)
                    ctx.arc(ccx, ccy, oR, qA0, qA1)
                    ctx.closePath()

                    ctx.fillStyle = isSelQ ? "rgba(224,120,38," + (0.85 * qualOp) + ")"
                                  : isHovQ ? "rgba(224,120,38," + (0.28 * qualOp) + ")"
                                  :          "rgba(16,20,32,"   + (0.78 * qualOp) + ")"
                    ctx.fill()
                    ctx.strokeStyle = (isSelQ || isHovQ)
                                      ? "rgba(224,120,38," + qualOp + ")"
                                      : "rgba(255,255,255," + (0.05 * qualOp) + ")"
                    ctx.lineWidth   = isSelQ ? 1.5 : 1
                    ctx.stroke()

                    // Label
                    var qMid  = (qA0 + qA1) / 2
                    var qLblR = (oR + iR) / 2
                    var qlx   = ccx + Math.cos(qMid) * qLblR
                    var qly   = ccy + Math.sin(qMid) * qLblR
                    var qfs   = Math.max(9, Math.min(14, oR * 0.11))
                    ctx.font         = (isSelQ ? "700 " : "500 ") + qfs + "px '" + fnt + "'"
                    ctx.textAlign    = "center"
                    ctx.textBaseline = "middle"
                    ctx.fillStyle    = isSelQ ? "rgba(255,255,255," + qualOp + ")"
                                     : isHovQ ? "rgba(255,212,160," + qualOp + ")"
                                     :          "rgba(200,212,228,"  + (0.80 * qualOp) + ")"
                    ctx.fillText(cs.qualityShort[q], qlx, qly)
                }

                // ── Vertical divider ──────────────────────────────────────
                ctx.strokeStyle = "rgba(255,255,255,0.10)"
                ctx.lineWidth   = 1
                ctx.beginPath()
                ctx.moveTo(ccx, ccy - oR)
                ctx.lineTo(ccx, ccy + oR)
                ctx.stroke()

                // ── Centre hole ───────────────────────────────────────────
                ctx.beginPath()
                ctx.arc(ccx, ccy, iR, 0, Math.PI * 2)
                ctx.fillStyle = "rgba(10,12,18,0.94)"
                ctx.fill()
                ctx.strokeStyle = rootActive ? "rgba(224,120,38," + (0.45 * qualOp + 0.10 * (1 - qualOp)) + ")" : "rgba(255,255,255,0.10)"
                ctx.lineWidth   = 1.5
                ctx.stroke()

                // Centre chord name
                ctx.textAlign    = "center"
                ctx.textBaseline = "middle"
                if (rootActive) {
                    var nameFs = Math.max(13, iR * 0.32)
                    ctx.font      = "700 " + nameFs + "px '" + fnt + "'"
                    ctx.fillStyle = "#E07826"
                    ctx.fillText(cs.roots[sR], ccx, ccy - iR * 0.18)
                    var qualFs = Math.max(10, iR * 0.25)
                    ctx.font      = "500 " + qualFs + "px '" + fnt + "'"
                    ctx.fillStyle = "rgba(220,228,240,0.90)"
                    ctx.fillText(cs.qualityShort[sQ], ccx, ccy + iR * 0.22)
                } else {
                    ctx.font      = "400 " + Math.max(9, iR * 0.22) + "px '" + fnt + "'"
                    ctx.fillStyle = "rgba(120,132,155,0.55)"
                    ctx.fillText("chord", ccx, ccy)
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
                font.family: figTreeVariable.name
                font.pixelSize: 14
                verticalAlignment: Text.AlignVCenter
                leftPadding: 12
            }
        }

        Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.06) }

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
        SettingsSlider {
            label: "Master Volume"
            unit: "%"
            from: 0; to: 100; stepSize: 1
            width: parent.width
            value: root.engineReady && appEngine.viewState ? (appEngine.viewState.masterVolume * 100) : 100
            onValueChanged: { if (root.engineReady) appEngine.setMasterVolume(value / 100.0) }
        }

        Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.06) }

    }
}
