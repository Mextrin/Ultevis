import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: parent ? parent.width : 280
    height: 32

    property string label: ""
    property alias checked: switchControl.checked

    Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        color: "#949AA5"
        font.pixelSize: 12
        font.weight: Font.Medium
    }

    Switch {
        id: switchControl
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        checked: false

        indicator: Rectangle {
            implicitWidth: 36
            implicitHeight: 20
            x: switchControl.leftPadding
            y: parent.height / 2 - height / 2
            radius: 10
            color: switchControl.checked ? "#E07A26" : Qt.rgba(1, 1, 1, 0.1)
            border.color: switchControl.checked ? "#E07A26" : Qt.rgba(1, 1, 1, 0.2)
            Behavior on color { ColorAnimation { duration: 150 } }

            Rectangle {
                x: switchControl.checked ? parent.width - width - 2 : 2
                y: 2
                width: 16
                height: 16
                radius: 8
                color: "#EBEDF0"
                Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.OutBack } }
            }
        }
    }
}
