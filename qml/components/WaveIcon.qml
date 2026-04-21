import QtQuick

// Minimal inline icon for a waveform shape ("sine" | "square" | "triangle" | "sawtooth").
// Draws via Canvas so we don't need SVG assets for the options.
Canvas {
    id: root
    property string shape: "sine"
    property color strokeColor: "#EBEDF0"
    property real strokeWidth: 1.6

    width: 24
    height: 16

    onShapeChanged: requestPaint()
    onStrokeColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d")
        ctx.reset()
        ctx.lineWidth = root.strokeWidth
        ctx.strokeStyle = root.strokeColor
        ctx.lineCap = "round"
        ctx.lineJoin = "round"

        const pad = 2
        const w = root.width - pad * 2
        const h = root.height - pad * 2
        const midY = pad + h / 2

        ctx.beginPath()
        if (root.shape === "sine") {
            const steps = 40
            for (let i = 0; i <= steps; ++i) {
                const t = i / steps
                const x = pad + t * w
                const y = midY - Math.sin(t * Math.PI * 2) * (h / 2)
                if (i === 0) ctx.moveTo(x, y)
                else ctx.lineTo(x, y)
            }
        } else if (root.shape === "square") {
            const qx = w / 4
            ctx.moveTo(pad, midY + h / 2)
            ctx.lineTo(pad, midY - h / 2)
            ctx.lineTo(pad + qx * 2, midY - h / 2)
            ctx.lineTo(pad + qx * 2, midY + h / 2)
            ctx.lineTo(pad + qx * 4, midY + h / 2)
            ctx.lineTo(pad + qx * 4, midY - h / 2)
        } else if (root.shape === "triangle") {
            ctx.moveTo(pad, midY)
            ctx.lineTo(pad + w * 0.25, midY - h / 2)
            ctx.lineTo(pad + w * 0.75, midY + h / 2)
            ctx.lineTo(pad + w, midY)
        } else if (root.shape === "sawtooth") {
            ctx.moveTo(pad, midY)
            ctx.lineTo(pad + w * 0.25, midY - h / 2)
            ctx.lineTo(pad + w * 0.25, midY + h / 2)
            ctx.lineTo(pad + w * 0.75, midY - h / 2)
            ctx.lineTo(pad + w * 0.75, midY + h / 2)
            ctx.lineTo(pad + w, midY)
        }
        ctx.stroke()
    }
}
