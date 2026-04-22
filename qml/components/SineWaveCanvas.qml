import QtQuick

Canvas {
    id: canvas
    anchors.fill: parent

    property real phase: 0

    NumberAnimation on phase {
        from: 0
        to: 2 * Math.PI
        duration: 6000
        loops: Animation.Infinite
    }

    onPhaseChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)

        var waves = [
            { amplitude: 40, frequency: 0.008, phaseOffset: 0,    yOffset: 0.35, opacity: 0.08, lineWidth: 2 },
            { amplitude: 55, frequency: 0.005, phaseOffset: 1.2,  yOffset: 0.50, opacity: 0.06, lineWidth: 1.5 },
            { amplitude: 30, frequency: 0.012, phaseOffset: 2.8,  yOffset: 0.65, opacity: 0.10, lineWidth: 1.5 },
            { amplitude: 45, frequency: 0.006, phaseOffset: 4.0,  yOffset: 0.80, opacity: 0.05, lineWidth: 2 }
        ]

        for (var w = 0; w < waves.length; w++) {
            var wave = waves[w]
            ctx.beginPath()
            ctx.strokeStyle = Qt.rgba(0.204, 0.596, 0.859, wave.opacity)
            ctx.lineWidth = wave.lineWidth

            var baseY = height * wave.yOffset
            for (var x = 0; x <= width; x += 3) {
                var y = baseY + Math.sin(x * wave.frequency + phase + wave.phaseOffset) * wave.amplitude
                if (x === 0) ctx.moveTo(x, y)
                else ctx.lineTo(x, y)
            }
            ctx.stroke()
        }
    }
}
