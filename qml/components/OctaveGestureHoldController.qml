import QtQuick

Item {
    id: root
    visible: false

    property int inputDelta: 0
    property int heldDelta: 0
    property int holdDelayMs: 2000
    property int repeatIntervalMs: 1000

    signal octaveChange(int delta)

    function resetHold() {
        holdDelayTimer.stop()
        holdRepeatTimer.stop()
        heldDelta = 0
    }

    function startHold(delta) {
        holdDelayTimer.stop()
        holdRepeatTimer.stop()
        heldDelta = delta
        octaveChange(delta)
        holdDelayTimer.restart()
    }

    onInputDeltaChanged: {
        if (inputDelta === 0)
            resetHold()
        else if (inputDelta !== heldDelta)
            startHold(inputDelta)
    }

    Timer {
        id: holdDelayTimer
        interval: root.holdDelayMs
        repeat: false
        onTriggered: {
            if (root.inputDelta === root.heldDelta && root.heldDelta !== 0)
                holdRepeatTimer.restart()
            else
                root.resetHold()
        }
    }

    Timer {
        id: holdRepeatTimer
        interval: root.repeatIntervalMs
        repeat: true
        onTriggered: {
            if (root.inputDelta === root.heldDelta && root.heldDelta !== 0)
                root.octaveChange(root.heldDelta)
            else
                root.resetHold()
        }
    }
}
