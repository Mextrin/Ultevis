import QtQuick
import QtQuick.Controls
import "components"

Item {
    id: root
    signal back()

    readonly property bool engineReady: typeof appEngine !== "undefined"

    FontLoader {
        id: figTreeVariable
        source: "../assets/fonts/Figtree-VariableFont_wght.ttf"
    }

    Rectangle {
        anchors.fill: parent
        color: "#101218"
    }

    // --- Header ---
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
            width: 48; height: 48
            anchors.left: parent.left; anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: root.back()
            Text {
                anchors.centerIn: parent
                text: "\u2190"
                font.pixelSize: 24
                color: backBtn.containsMouse ? "#E07A26" : "#949AA5"
            }
        }

        MouseArea {
            id: settingsBtn
            width: 40; height: 40
            anchors.left: backBtn.right; anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: settingsPane.open = !settingsPane.open
            Rectangle {
                anchors.fill: parent; radius: 20
                color: settingsBtn.containsMouse || settingsPane.open ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
                Image {
                    anchors.centerIn: parent; width: 22; height: 22
                    source: "qrc:/assets/icons/settings.svg"
                    opacity: settingsBtn.containsMouse || settingsPane.open ? 1.0 : 0.85
                }
            }
        }

        Text {
            anchors.centerIn: parent
            text: "Theremin"
            font.family: figTreeVariable.name
            font.pixelSize: 20; font.weight: Font.Light; font.letterSpacing: 2
            color: "#E07826"
        }

        WaveShapeDropdown {
            id: waveDropdown
            anchors.right: parent.right; anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            value: root.engineReady && appEngine.viewState && appEngine.viewState.thereminWaveform !== undefined ? appEngine.viewState.thereminWaveform : "sine"
            font.family: figTreeVariable.name
            onChanged: function(v) { if (root.engineReady) appEngine.setThereminWaveform(v) }
        }
    }

    // --- Video Feed ---
    Image {
        id: cameraFeed
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        fillMode: Image.PreserveAspectFit
        source: "image://camera/feed"
        cache: false
        smooth: true
        mipmap: true

        Timer {
            interval: 33; running: true; repeat: true
            onTriggered: cameraFeed.source = "image://camera/feed?id=" + Math.random()
        }
    }

    // --- Interaction Layer ---
    Item {
        id: interactionLayer
        width: cameraFeed.paintedWidth
        height: cameraFeed.paintedHeight
        anchors.centerIn: cameraFeed
        z: 5
        opacity: 0.85 

        Canvas {
            id: guideLine
            anchors.fill: parent
            onWidthChanged: requestPaint(); onHeightChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d"); ctx.reset(); ctx.save()
                ctx.strokeStyle = Qt.rgba(0.922, 0.929, 0.941, 0.6); ctx.lineWidth = 4; ctx.setLineDash([6, 8])
                const x = Math.round(width * 0.365) + 0.5
                ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, height); ctx.stroke(); ctx.restore()
            }
        }

        Canvas {
            id: trailCanvas
            anchors.fill: parent
            property var leftTrail: []
            property var rightTrail: []
            readonly property int maxTrailLength: 40

            Connections {
                target: root.engineReady ? appEngine : null
                function onHandStateChanged() {
                    if (!root.engineReady) return;

                    if (appEngine.rightHandVisible) {
                        trailCanvas.rightTrail.push({x: (1.0 - appEngine.rightHandX) * trailCanvas.width, y: appEngine.rightHandY * trailCanvas.height})
                        if (trailCanvas.rightTrail.length > trailCanvas.maxTrailLength) trailCanvas.rightTrail.shift()
                    } else trailCanvas.rightTrail = []

                    if (appEngine.leftHandVisible) {
                        trailCanvas.leftTrail.push({x: (1.0 - appEngine.leftHandX) * trailCanvas.width, y: appEngine.leftHandY * trailCanvas.height})
                        if (trailCanvas.leftTrail.length > trailCanvas.maxTrailLength) trailCanvas.leftTrail.shift()
                    } else trailCanvas.leftTrail = []

                    trailCanvas.requestPaint()
                }
            }

            onPaint: {
                var ctx = getContext("2d"); 
                ctx.clearRect(0, 0, width, height); 
                ctx.lineCap = "round";
                
                // --- THE NEON TRICK ---
                // Additive blending makes the trail literally glow when it overlaps itself
                ctx.globalCompositeOperation = "lighter"; 
                
                function drawFadingTrail(trail) {
                    if (trail.length < 2) return;
                    for (var i = 1; i < trail.length; i++) {
                        ctx.beginPath(); 
                        ctx.moveTo(trail[i-1].x, trail[i-1].y); 
                        ctx.lineTo(trail[i].x, trail[i].y);
                        
                        var alpha = i / trail.length;
                        
                        // Pure Electric Orange with lowered max opacity (0.4 instead of 0.7)
                        ctx.strokeStyle = "rgba(255, 100, 0, " + (alpha * 0.4) + ")";
                        ctx.lineWidth = 14 * alpha; 
                        ctx.stroke();
                    }
                }
                drawFadingTrail(leftTrail); 
                drawFadingTrail(rightTrail);
            }
        }

        Rectangle {
            visible: root.engineReady && appEngine.leftHandVisible
            width: 18; height: 18; radius: 9
            x: (root.engineReady ? (1.0 - appEngine.leftHandX) * parent.width : 0) - width / 2
            y: (root.engineReady ? appEngine.leftHandY * parent.height : 0) - height / 2
            color: "#e07826"
            border.color: "#FFFFFF"
            border.width: 2
            Behavior on color { ColorAnimation { duration: 100 } }
        }

        Rectangle {
            visible: root.engineReady && appEngine.rightHandVisible
            width: 18; height: 18; radius: 9
            x: (root.engineReady ? (1.0 - appEngine.rightHandX) * parent.width : 0) - width / 2
            y: (root.engineReady ? appEngine.rightHandY * parent.height : 0) - height / 2
            color: "#e07826"
            border.color: "#FFFFFF"
            border.width: 2
            Behavior on color { ColorAnimation { duration: 100 } }
        }
    }

    // --- Settings Pane Overlay ---
    MouseArea {
        anchors.top: header.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        visible: settingsPane.open
        onClicked: settingsPane.open = false
        z: 20
    }

    SettingsPane {
        id: settingsPane
        property bool open: false
        anchors.left: parent.left; anchors.leftMargin: 20
        anchors.top: header.bottom; anchors.topMargin: 12
        width: 300; z: 30
        font.family: figTreeVariable.name
        onClose: open = false
        visible: open
        opacity: open ? 1.0 : 0.0
        scale: open ? 1.0 : 0.96
        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
        Behavior on scale   { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
    }
}