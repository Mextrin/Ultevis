import QtQuick
import QtQuick.Controls

Item {
    id: placeholder
    signal back()

    Rectangle {
        anchors.fill: parent
        color: "#101218"
    }

    Column {
        anchors.centerIn: parent
        spacing: 24

        Text {
            text: "Session"
            font.pixelSize: 36
            font.weight: Font.Light
            color: "#EBEDF0"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "Coming soon..."
            font.pixelSize: 16
            color: "#949AA5"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Item { width: 1; height: 16 }

        Rectangle {
            width: 160
            height: 44
            radius: 8
            color: backArea.containsMouse ? "#42A5E3" : "#3498DB"
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                anchors.centerIn: parent
                text: "Back"
                font.pixelSize: 16
                font.weight: Font.Medium
                color: "#FFFFFF"
            }

            MouseArea {
                id: backArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: placeholder.back()
            }
        }
    }
}
