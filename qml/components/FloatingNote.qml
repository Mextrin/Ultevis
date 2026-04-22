import QtQuick

Text {
    id: note

    property real startX: Math.random() * (parent ? parent.width : 800)
    property real noteSize: 20 + Math.random() * 40
    property real floatDuration: 8000 + Math.random() * 7000
    property real swayAmount: 20 + Math.random() * 30
    property real swayDuration: 3000 + Math.random() * 2000
    property real rotationAmount: -15 + Math.random() * 30
    property real noteOpacity: 0.06 + Math.random() * 0.12

    property var notes: ["\u266A", "\u266B", "\u266C", "\u2669"]
    text: notes[Math.floor(Math.random() * notes.length)]

    font.pixelSize: noteSize
    color: Qt.rgba(0.204, 0.596, 0.859, noteOpacity)
    x: startX
    y: parent ? parent.height + 20 : 820
    rotation: 0

    SequentialAnimation on y {
        loops: Animation.Infinite
        NumberAnimation {
            to: -80
            duration: note.floatDuration
            easing.type: Easing.Linear
        }
        ScriptAction {
            script: {
                note.startX = Math.random() * (note.parent ? note.parent.width : 800)
                note.x = note.startX
            }
        }
        NumberAnimation {
            to: note.parent ? note.parent.height + 20 : 820
            duration: 0
        }
    }

    SequentialAnimation on x {
        loops: Animation.Infinite
        NumberAnimation {
            to: note.startX + note.swayAmount
            duration: note.swayDuration
            easing.type: Easing.InOutSine
        }
        NumberAnimation {
            to: note.startX - note.swayAmount
            duration: note.swayDuration
            easing.type: Easing.InOutSine
        }
    }

    NumberAnimation on rotation {
        from: -rotationAmount
        to: rotationAmount
        duration: swayDuration * 2
        loops: Animation.Infinite
        easing.type: Easing.InOutSine
    }
}
