import QtQuick
import QtQuick.Controls
import "components" // Ensures QML can find your DrumSettingsPane.qml

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
        
        fillMode: Image.PreserveAspectFit 

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

        // --- NEW: Settings gear ---
        MouseArea {
            id: settingsBtn
            width: 40
            height: 40
            anchors.left: backBtn.right
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            
            // Toggles the pane open/closed!
            onClicked: drumSettings.open = !drumSettings.open

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: settingsBtn.containsMouse || drumSettings.open
                       ? Qt.rgba(1, 1, 1, 0.08)
                       : "transparent"
                Behavior on color { ColorAnimation { duration: 150 } }

                Image {
                    anchors.centerIn: parent
                    width: 22
                    height: 22
                    source: "qrc:/assets/icons/settings.svg"
                    sourceSize.width: 22
                    sourceSize.height: 22
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    opacity: settingsBtn.containsMouse || drumSettings.open ? 1.0 : 0.85
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }
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

    // --- NEW: Settings pane overlay ---
    MouseArea {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: drumSettings.open
        onClicked: drumSettings.open = false
        z: 20
    }

    // --- NEW: Drum Settings Pane ---
    DrumSettingsPane {
        id: drumSettings
        property bool open: false

        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: header.bottom
        anchors.topMargin: 12
        width: 300
        z: 30

        font.family: figTreeVariable.name
        
        onClose: open = false

        visible: open
        opacity: open ? 1.0 : 0.0
        scale: open ? 1.0 : 0.96
        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
        Behavior on scale   { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
    }
}