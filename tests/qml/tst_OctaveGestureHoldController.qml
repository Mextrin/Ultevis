import QtQuick
import QtTest
import "../../qml/components"

TestCase {
    id: testCase
    name: "OctaveGestureHoldController"

    property int changeCount: 0
    property int lastDelta: 0

    function resetCounters() {
        changeCount = 0
        lastDelta = 0
    }

    function createController() {
        const controller = createTemporaryObject(controllerComponent, testCase)
        verify(controller !== null)
        controller.octaveChange.connect(function(delta) {
            testCase.changeCount += 1
            testCase.lastDelta = delta
        })
        return controller
    }

    function waitForChangeCount(expectedCount, timeoutMs) {
        let waitedMs = 0
        while (changeCount < expectedCount && waitedMs < timeoutMs) {
            wait(10)
            waitedMs += 10
        }
        compare(changeCount, expectedCount)
    }

    Component {
        id: controllerComponent

        OctaveGestureHoldController {
            holdDelayMs: 40
            repeatIntervalMs: 30
        }
    }

    function test_initial_change_then_repeats_after_hold_delay() {
        resetCounters()
        const controller = createController()

        controller.inputDelta = 1

        compare(changeCount, 1)
        compare(lastDelta, 1)
        wait(35)
        compare(changeCount, 1)
        waitForChangeCount(2, 120)
        compare(lastDelta, 1)
    }

    function test_same_gesture_does_not_need_alternating_to_repeat() {
        resetCounters()
        const controller = createController()

        controller.inputDelta = -1

        compare(changeCount, 1)
        compare(lastDelta, -1)
        waitForChangeCount(3, 180)
        compare(lastDelta, -1)
    }

    function test_disappearing_gesture_resets_hold_timer() {
        resetCounters()
        const controller = createController()

        controller.inputDelta = 1
        compare(changeCount, 1)
        controller.inputDelta = 0
        wait(100)
        compare(changeCount, 1)

        controller.inputDelta = 1
        compare(changeCount, 2)
        wait(35)
        compare(changeCount, 2)
    }

    function test_direction_change_restarts_with_immediate_change() {
        resetCounters()
        const controller = createController()

        controller.inputDelta = 1
        compare(changeCount, 1)
        compare(lastDelta, 1)

        controller.inputDelta = -1
        compare(changeCount, 2)
        compare(lastDelta, -1)
        wait(35)
        compare(changeCount, 2)
    }
}
