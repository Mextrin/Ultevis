import QtQuick

Item {
    id: card

    property string instrumentName: ""
    property string iconSource: ""
    signal clicked()

    // Hover state
    property bool hovered: mouseArea.containsMouse

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: 16
        color: card.hovered ? Qt.rgba(0.878, 0.478, 0.149, 0.18) : Qt.rgba(1, 1, 1, 0.025)
        border.width: 1
        border.color: card.hovered ? Qt.rgba(0.878, 0.478, 0.149, 0.4) : Qt.rgba(1, 1, 1, 0.04)

        Behavior on color { ColorAnimation { duration: 250 } }
        Behavior on border.color { ColorAnimation { duration: 250 } }

        // Scale on hover
        transform: Scale {
            id: scaleTransform
            origin.x: bg.width / 2
            origin.y: bg.height / 2
            xScale: card.hovered ? 1.025 : 1.0
            yScale: card.hovered ? 1.025 : 1.0
            Behavior on xScale { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
            Behavior on yScale { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
        }

        Column {
            anchors.centerIn: parent
            spacing: 16

            // Instrument icon
            Image {
                source: card.iconSource
                width: 72
                height: 72
                sourceSize.width: 72
                sourceSize.height: 72
                anchors.horizontalCenter: parent.horizontalCenter
                opacity: card.hovered ? 1.0 : 0.7
                Behavior on opacity { NumberAnimation { duration: 250 } }
            }

            // Instrument name
            Text {
                text: card.instrumentName
                font.pixelSize: 22
                font.weight: Font.Normal
                font.letterSpacing: 2
                color: card.hovered ? "#E07A26" : "#EBEDF0"
                anchors.horizontalCenter: parent.horizontalCenter
                Behavior on color { ColorAnimation { duration: 250 } }
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: card.clicked()
    }
}
