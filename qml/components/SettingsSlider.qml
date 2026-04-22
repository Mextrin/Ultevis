import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    width: parent ? parent.width : 280
    height: 48

    property string label: ""
    property string unit: ""
    property real from: 0
    property real to: 100
    property real value: 0
    property real stepSize: 1

    Text {
        anchors.left: parent.left
        anchors.top: parent.top
        text: root.label
        color: "#949AA5"
        font.pixelSize: 12
        font.weight: Font.Medium
    }

    Text {
        anchors.right: parent.right
        anchors.top: parent.top
        text: root.value.toFixed(root.stepSize % 1 === 0 ? 0 : 2) + root.unit
        color: "#EBEDF0"
        font.pixelSize: 12
        font.weight: Font.DemiBold
    }

    Slider {
        id: slider
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 24
        from: root.from
        to: root.to
        stepSize: root.stepSize
        value: root.value
        onValueChanged: root.value = value

        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: slider.availableWidth
            height: 4
            radius: 2
            color: Qt.rgba(1, 1, 1, 0.08)

            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                color: "#E07A26"
                radius: 2
            }
        }

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: 16
            height: 16
            radius: 8
            color: slider.pressed ? "#E07A26" : "#EBEDF0"
            border.color: "#E07A26"
            border.width: slider.pressed ? 2 : 1
            Behavior on color { ColorAnimation { duration: 150 } }
        }
    }
}
