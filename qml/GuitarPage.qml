pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import "components"

Item {
    id: root
    signal back()

    readonly property bool engineReady: typeof appEngine !== "undefined"

    property int  neckPosition:   0   
    property int  neckFlashDelta: 0   
    property bool strumFlash:     false

    property bool prevThumbUp:   false
    property bool prevThumbDown: false
    property bool neckCooldown:  false

    property bool sharpsEnabled:        false
    property int  selectedChordRoot:    -1   // 0 to 11 globally linked to C++
    property int  selectedChordQuality:  0   

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

    Timer {
        id: neckCooldownTimer
        interval: 700
        repeat:   false
        onTriggered: root.neckCooldown = false
    }

    Connections {
        target: root.engineReady ? appEngine : null
        function onHandStateChanged() {
            if (!root.engineReady) return

            const up   = root.anyThumbUpInside(rightHalf)
            const down = root.anyThumbDownInside(rightHalf)

            if (!root.neckCooldown) {
                if (up && !root.prevThumbUp) {
                    if (appEngine.adjustGuitarVoicing) appEngine.adjustGuitarVoicing(1)
                    
                    root.neckPosition   = Math.min(2, root.neckPosition + 1)
                    root.neckFlashDelta = +1
                    neckFlashTimer.restart()
                    root.neckCooldown = true
                    neckCooldownTimer.restart()
                } else if (down && !root.prevThumbDown) {
                    if (appEngine.adjustGuitarVoicing) appEngine.adjustGuitarVoicing(-1)
                    
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

    FontLoader {
        id: figTreeVariable
        source: "../assets/fonts/Figtree-VariableFont_wght.ttf"
    }

    Rectangle {
        anchors.fill: parent
        color: "#101218"
    }

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

        Text {
            id: loadingText
            anchors.centerIn: parent
            text: "Initializing Camera..."
            font.family: figTreeVariable.name
            font.pixelSize: 18
            font.weight: Font.Medium
            color: "#949AA5"
            
            SequentialAnimation on opacity {
                PauseAnimation { duration: 2500 }
                NumberAnimation { to: 0.0; duration: 500 }
            }
        }

        Timer {
            interval: 33
            running:  true
            repeat:   true
            onTriggered: cameraFeed.source = "image://camera/feed?id=" + Math.random()
        }
    }

    Item {
        id: cameraViewport
        readonly property real contentWidth:  cameraFeed.paintedWidth  > 0 ? cameraFeed.paintedWidth  : cameraFeed.width
        readonly property real contentHeight: cameraFeed.paintedHeight > 0 ? cameraFeed.paintedHeight : cameraFeed.height
        x:      cameraFeed.x + (cameraFeed.width  - contentWidth)  / 2
        y:      cameraFeed.y + (cameraFeed.height - contentHeight) / 2
        width:  contentWidth
        height: contentHeight
        z: 5

        Item {
            id: guitarOverlay
            anchors.fill: parent

            ChordSelector {
                x:      0
                y:      0
                width:  parent.width  * 0.50
                height: parent.height
            }

            Item {
                id: rightHalf
                x:      parent.width * 0.50
                y:      0
                width:  parent.width  * 0.50
                height: parent.height

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
                                  ? ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"][root.selectedChordRoot]
                                    + "  "
                                    + ["Maj","Min","7","Maj7","Min7","Sus2","Sus4"][root.selectedChordQuality]
                                  : ""
                            font.family:    figTreeVariable.name
                            font.pixelSize: 40
                            font.weight:    Font.Bold
                            color:          "#E07826"
                        }
                    }
                }

                StrumZone {
                    id: strumZone
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    anchors.rightMargin: 20
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 30
                    height: parent.height * 0.5
                }

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

    component StrumZone: Item {
        id: sz

        Connections {
            target: root.engineReady ? appEngine : null
            function onHandStateChanged() {
                if (root.engineReady && appEngine.guitarStrumHit) {
                    root.strumFlash = true
                    strumFlashTimer.restart()
                }
            }
        }

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

                var shCX = W * 0.62
                var shR  = Math.min(W * 0.26, H * 0.44)

                ctx.strokeStyle = "rgba(255,255,255,0.02)"
                ctx.lineWidth   = 1
                ctx.beginPath(); ctx.arc(shCX, cy, shR + 14, 0, Math.PI * 2); ctx.stroke()

                ctx.strokeStyle = "rgba(224,120,38,0.08)"
                ctx.lineWidth   = 1
                ctx.beginPath(); ctx.arc(shCX, cy, shR + 10, 0, Math.PI * 2); ctx.stroke()

                ctx.strokeStyle = "rgba(224,120,38,0.15)"
                ctx.lineWidth   = 1.5
                ctx.beginPath(); ctx.arc(shCX, cy, shR + 4,  0, Math.PI * 2); ctx.stroke()

                ctx.fillStyle = "rgba(4, 3, 5, 0.40)"
                ctx.beginPath(); ctx.arc(shCX, cy, shR, 0, Math.PI * 2); ctx.fill()

                ctx.strokeStyle = "rgba(224,120,38,0.25)"
                ctx.lineWidth   = 1.5
                ctx.beginPath(); ctx.arc(shCX, cy, shR, 0, Math.PI * 2); ctx.stroke()

                var saddleX = W * 0.10
                var saddleH = H * 0.60
                ctx.fillStyle = "rgba(205,215,228,0.55)"
                ctx.fillRect(saddleX - 1.5, cy - saddleH * 0.5, 3, saddleH)

                var numStrings = 6
                var strSpacing = H * 0.11
                var s0y        = cy - (numStrings - 1) * strSpacing * 0.5

                for (var s = 0; s < numStrings; s++) {
                    var sy = s0y + s * strSpacing
                    ctx.strokeStyle = "rgba(195,208,224,0.65)"
                    ctx.lineWidth   = 1.8 + (numStrings - 1 - s) * 0.45
                    ctx.beginPath()
                    ctx.moveTo(bodyLeft,  sy)
                    ctx.lineTo(bodyRight, sy)
                    ctx.stroke()
                }
            }
        }
    }

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

    component FretboardBars: Item {
        id: fb

        Canvas {
            id: fbCanvas
            anchors.fill: parent

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

                var scale24 = 1.0 - Math.pow(0.5, 24.0 / 12.0)
                function fretX(n) {
                    return (1.0 - Math.pow(0.5, n / 12.0)) / scale24 * W
                }

                var zoneBounds = [[0, 7], [7, 15], [15, 24]]

                ctx.fillStyle = "rgba(14,16,22,0.92)"
                ctx.fillRect(0, 0, W, H)

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

                for (var f = 0; f <= numFrets; f++) {
                    var fx = fretX(f)
                    if (f === 0) {
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

                ctx.strokeStyle = "rgba(255,255,255,0.08)"
                ctx.lineWidth   = 1
                ctx.strokeRect(0.5, 0.5, W - 1, H - 1)
            }
        }
    }

    component ChordSelector: Item {
        id: cs

        readonly property var naturalRoots: ["C","D","E","F","G","A","B"]
        readonly property var naturalIndices: [0, 2, 4, 5, 7, 9, 11]
        
        readonly property var sharpRoots:   ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"]
        readonly property var sharpIndices: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]

        readonly property var roots:       root.sharpsEnabled ? sharpRoots : naturalRoots
        readonly property var rootIndices: root.sharpsEnabled ? sharpIndices : naturalIndices
        readonly property int numRoots:    roots.length

        readonly property var qualities: ["Maj","Min","7","Maj7","Min7","Sus2","Sus4"]
        readonly property int numQ:      7

        // --- THE FIX: Move the column further right and let the wheel auto-scale ---
        readonly property real colX:   width * 0.28   
        readonly property real colW:   width * 0.18   
        readonly property real semiCX: colX + colW    
        readonly property real semiCY: height * 0.50  
        readonly property real rowH:   height / numRoots
        readonly property real semiR:  Math.min((width - semiCX) * 0.92, height * 0.46)
        readonly property real innerR: semiR * 0.28
        // ---------------------------------------------------------------------------

        property int  hovRoot:     -1
        property bool dragging:    false
        property bool prevLPinch:  false

        function localPt(nx, ny) {
            return cs.mapFromItem(cameraViewport,
                                  nx * cameraViewport.width,
                                  ny * cameraViewport.height)
        }

        function rootAtY(py) {
            return Math.max(0, Math.min(cs.numRoots - 1, Math.floor(py / cs.height * cs.numRoots)))
        }

        function qualityFromAngle(dx, dy) {
            var angle = Math.atan2(dy, dx)
            var norm  = (angle + Math.PI / 2) / Math.PI
            return Math.max(0, Math.min(cs.numQ - 1, Math.floor(norm * cs.numQ)))
        }

        Connections {
            target: root.engineReady ? appEngine : null

            function onHandStateChanged() {
                if (!root.engineReady || !appEngine.leftHandVisible) {
                    if (cs.dragging) {
                        root.selectedChordRoot = -1
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
                        if (root.selectedChordRoot >= 0) {
                            var dx   = pt.x - cs.semiCX
                            var dy   = pt.y - cs.semiCY
                            
                            // --- THE FIX: Calculate the exact distance from the center ---
                            var dist = Math.sqrt(dx * dx + dy * dy)
                            
                            // --- THE FIX: Strict boundaries. Must be exactly inside the drawn ring! ---
                            if (dx >= 0 && dist >= cs.innerR && dist <= cs.semiR) {
                                var newQuality = cs.qualityFromAngle(dx, dy)
                                if (root.selectedChordQuality !== newQuality) {
                                    root.selectedChordQuality = newQuality
                                    
                                    if (appEngine.setGuitarChord) {
                                        appEngine.setGuitarChord(root.selectedChordRoot, newQuality)
                                    }
                                }
                            }
                        }
                    } else {
                        root.selectedChordRoot = -1
                        cs.dragging = false
                    }
                } else {
                    var inCol = pt.x >= cs.colX && pt.x <= cs.colX + cs.colW && pt.y >= 0 && pt.y <= cs.height
                    cs.hovRoot = inCol ? cs.rootAtY(pt.y) : -1

                    if (lp && !cs.prevLPinch && inCol) {
                        var ri = cs.rootAtY(pt.y)
                        var actualRootIndex = cs.rootIndices[ri]
                        
                        root.selectedChordRoot = actualRootIndex
                        root.selectedChordQuality = 0 
                        cs.dragging = true
                        
                        if (appEngine.setGuitarChord) {
                            appEngine.setGuitarChord(actualRootIndex, 0)
                        }
                    }
                }

                cs.prevLPinch = lp
            }
        }

        Canvas {
            id: csCanvas
            anchors.fill: parent

            property int    pSelRoot:   root.selectedChordRoot
            property int    pSelQual:   root.selectedChordQuality
            property int    pHovRoot:   cs.hovRoot
            property bool   pDragging:  cs.dragging
            property int    pNumRoots:  cs.numRoots
            property bool   pSharps:    root.sharpsEnabled
            property string pFont:      figTreeVariable.name

            onPSelRootChanged:  requestPaint()
            onPSelQualChanged:  requestPaint()
            onPHovRootChanged:  requestPaint()
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
                var isDrag = cs.dragging
                var pad    = 3

                var scX = cs.semiCX  
                var scY = cs.semiCY  

                if (sR >= 0) {
                    ctx.beginPath()
                    ctx.moveTo(scX, scY)
                    ctx.arc(scX, scY, semiR, -Math.PI / 2, Math.PI / 2, false)
                    ctx.closePath()
                    ctx.fillStyle = "rgba(8,10,16,0.72)"
                    ctx.fill()

                    for (var q = 0; q < nQ; q++) {
                        var startA  = -Math.PI / 2 + (q / nQ) * Math.PI
                        var endA    = -Math.PI / 2 + ((q + 1) / nQ) * Math.PI
                        var isSelQ  = (q === sQ)

                        ctx.beginPath()
                        ctx.arc(scX, scY, semiR - 1, startA, endA)
                        ctx.arc(scX, scY, innerR + 1, endA, startA, true)
                        ctx.closePath()

                        ctx.fillStyle   = isSelQ ? "rgba(224,120,38,0.88)" : "rgba(20,24,34,0.80)"
                        ctx.fill()
                        ctx.strokeStyle = isSelQ ? "#E07826" : "rgba(255,255,255,0.08)"
                        ctx.lineWidth   = isSelQ ? 2 : 1
                        ctx.stroke()

                        var midA   = (startA + endA) / 2
                        var labelR = (semiR + innerR) / 2
                        var lx = scX + Math.cos(midA) * labelR
                        var ly = scY + Math.sin(midA) * labelR
                        var qfs = Math.max(9, Math.min(15, (semiR - innerR) * 0.24))
                        ctx.font         = (isSelQ ? "700 " : "500 ") + qfs + "px '" + csCanvas.pFont + "'"
                        ctx.textAlign    = "center"
                        ctx.textBaseline = "middle"
                        ctx.fillStyle    = isSelQ ? "#FFFFFF" : "rgba(190,202,218,0.80)"
                        ctx.fillText(cs.qualities[q], lx, ly)
                    }

                    ctx.beginPath()
                    ctx.moveTo(scX, scY)
                    ctx.arc(scX, scY, innerR, -Math.PI / 2, Math.PI / 2, false)
                    ctx.closePath()
                    ctx.fillStyle = "rgba(10,12,18,0.92)"
                    ctx.fill()

                    ctx.strokeStyle = "#E07826"
                    ctx.lineWidth   = 1.5
                    ctx.setLineDash([4, 4])
                    ctx.beginPath()
                    ctx.moveTo(scX, scY)
                    ctx.lineTo(scX + innerR, scY)
                    ctx.stroke()
                    ctx.setLineDash([])
                }

                for (var i = 0; i < n; i++) {
                    var ry    = i * rowH
                    
                    var isSel = (cs.rootIndices[i] === sR)
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

        GuitarDropdownImages {
            anchors.right:          parent.right
            anchors.rightMargin:    20
            anchors.verticalCenter: parent.verticalCenter
            font.family: figTreeVariable.name
            onChanged: function(key) {
                if (!root.engineReady) return
                const soundIds = { "acoustic_guitar": 2, "electric_guitar": 0, "distorted_guitar": 1 }
                appEngine.setGuitarSound(soundIds[key] ?? 2)
            }
        }
    }

    MouseArea {
        anchors.top:    header.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        visible: settingsPanel.open
        onClicked: settingsPanel.open = false
        z: 20
    }

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

         Text {
            text: "GUITAR SETTINGS"
            font.family: figTreeVariable.name
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.letterSpacing: 1.5
            color: "#E07826"
        }

        SettingsToggle {
            label:   "Show Sharps / Flats"
            checked: root.sharpsEnabled
            onCheckedChanged: root.sharpsEnabled = checked
        }

    }
}