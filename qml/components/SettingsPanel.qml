import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string title: "Settings"
    property bool open: false
    property alias font: titleLabel.font

    default property alias content: contentLayout.data

    width: 300
    // Height wraps content by default, up to a reasonable max based on parent.
    // If you need it constrained by the parent height, you can set constraints from outside.
    implicitHeight: Math.min(contentCol.implicitHeight + 32, parent ? parent.height - y - 20 : 800)

    visible: open || opacity > 0
    opacity: open ? 1.0 : 0.0
    scale: open ? 1.0 : 0.96
    Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
    Behavior on scale   { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: 10
        color: "#1A1D26"
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.08)

        // Soft drop shadow approximation (an offset dimmer layer behind the card).
        Rectangle {
            z: -1
            anchors.fill: parent
            anchors.leftMargin: -2
            anchors.rightMargin: -2
            anchors.topMargin: -2
            anchors.bottomMargin: -6
            radius: parent.radius + 2
            color: Qt.rgba(0, 0, 0, 0.35)
        }
    }

    ColumnLayout {
        id: contentCol
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        // Header with title + close
        Item {
            Layout.fillWidth: true
            implicitHeight: 20

            Text {
                id: titleLabel
                text: root.title
                color: "#EBEDF0"
                font.pixelSize: 15
                font.weight: Font.Medium
                font.letterSpacing: 1.5
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
            }

            MouseArea {
                id: closeBtn
                width: 20
                height: 20
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.open = false

                Text {
                    anchors.centerIn: parent
                    text: "\u00D7"
                    font.pixelSize: 18
                    color: closeBtn.containsMouse ? "#E07A26" : "#949AA5"
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
            }
        }

        // ScrollView for the settings controls
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            Column {
                id: contentLayout
                width: parent.width
                spacing: 16
            }
        }
    }
}
