import QtQuick
import QtQuick.Controls

Item {
    id: root
    signal back()

    FontLoader {
        id: figTreeVariable
        source: "../assets/fonts/Figtree-VariableFont_wght.ttf"
    }

    // Dark background
    Rectangle {
        anchors.fill: parent
        color: "#101218"
    }

    // --- Python Camera Feed ---
    Image {
        id: cameraFeed
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        
        // --- CHANGE THIS LINE ---
        fillMode: Image.PreserveAspectFit 
        // ------------------------

        source: "image://camera/feed"
        cache: false 
        
        // This property makes the scaling look much smoother/sharper
        smooth: true
        mipmap: true

        Timer {
            interval: 33
            running: true
            repeat: true
            onTriggered: {
                cameraFeed.source = "image://camera/feed?id=" + Math.random()
            }
        }
    }

    // --- Header ---
    Item {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        z: 10

        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0.063, 0.071, 0.094, 0.75)
        }

        // Back arrow
        MouseArea {
            id: backBtn
            width: 48
            height: 48
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            
            // This tells C++ to switch the state back to the Session Page!
            onClicked: root.back()

            Text {
                anchors.centerIn: parent
                text: "\u2190"
                font.pixelSize: 24
                color: backBtn.containsMouse ? "#E07A26" : "#949AA5"
                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }

        // Title
        Text {
            anchors.centerIn: parent
            text: "Drums"
            font.family: figTreeVariable.name
            font.pixelSize: 20
            font.weight: Font.Light
            font.letterSpacing: 2
            color: "#E07826"
        }
    }
}