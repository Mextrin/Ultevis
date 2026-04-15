import QtQuick
import QtQuick.Controls

ComboBox {
    id: root
    width: 160
    height: 32

    background: Rectangle {
        color: root.down ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(1, 1, 1, 0.05)
        border.color: root.activeFocus ? "#E07A26" : Qt.rgba(1, 1, 1, 0.1)
        border.width: 1
        radius: 4
        Behavior on border.color { ColorAnimation { duration: 150 } }
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    contentItem: Text {
        leftPadding: 12
        rightPadding: root.indicator.width + root.spacing
        text: root.displayText
        font.pixelSize: 12
        color: "#EBEDF0"
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Canvas {
        id: canvas
        x: root.width - width - root.rightPadding
        y: root.topPadding + (root.availableHeight - height) / 2
        width: 12
        height: 8
        contextType: "2d"

        Connections {
            target: root
            function onPressedChanged() { canvas.requestPaint(); }
        }

        onPaint: {
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width / 2, height);
            context.closePath();
            context.fillStyle = root.pressed ? "#E07A26" : "#949AA5";
            context.fill();
        }
    }

    delegate: ItemDelegate {
        width: root.width
        height: 32
        contentItem: Text {
            text: modelData
            color: root.highlightedIndex === index ? "#E07A26" : "#EBEDF0"
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: root.highlightedIndex === index ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
        }
    }

    popup: Popup {
        y: root.height - 1
        width: root.width
        implicitHeight: contentItem.implicitHeight + 8
        padding: 4

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: "#1A1D26"
            border.color: Qt.rgba(1, 1, 1, 0.15)
            radius: 4
        }
    }
}
