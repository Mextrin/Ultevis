import QtQuick
import QtQuick.Controls
import QtQuick.Window



ApplicationWindow {
    id: root
    width: 1280
    height: 800
    minimumWidth: 960
    minimumHeight: 600
    visible: true
    title: "Airchestra"
    color: "#101218"

    StackView {
        id: navigation
        anchors.fill: parent
        initialItem: landingPageComponent

        pushEnter: Transition {
            PropertyAnimation { property: "opacity"; from: 0; to: 1; duration: 500; easing.type: Easing.InOutQuad }
        }
        pushExit: Transition {
            PropertyAnimation { property: "opacity"; from: 1; to: 0; duration: 500; easing.type: Easing.InOutQuad }
        }
        popEnter: Transition {
            PropertyAnimation { property: "opacity"; from: 0; to: 1; duration: 500; easing.type: Easing.InOutQuad }
        }
        popExit: Transition {
            PropertyAnimation { property: "opacity"; from: 1; to: 0; duration: 500; easing.type: Easing.InOutQuad }
        }
    }

    Component {
        id: landingPageComponent
        LandingPage {
            onProceed: {
                appEngine.proceed()
                navigation.push(instrumentSelectComponent)
            }
        }
    }

    Component {
        id: instrumentSelectComponent
        InstrumentSelectPage {
            onBack: {
                appEngine.goBack()
                navigation.pop()
            }
            onInstrumentSelected: function(name) {
                appEngine.selectInstrument(name)
                if (name === "theremin") {
                    navigation.push(thereminComponent)
                }
                else if (name === "guitar") {
                    navigation.push(guitarPageComponent)
                }
                else if (name === "keyboard") {
                    navigation.push(keyboardPageComponent)
                } else if (name === "drums") {
                    navigation.push(drumsPageComponent)
                }
            }
        }
    }

    Component {
        id: thereminComponent
        ThereminPage {
            onBack: {
                appEngine.goBack()
                navigation.pop()
            }
        }
    }

    Component {
        id: guitarPageComponent
        GuitarPage {
            onBack: navigation.pop()
        }
    }

    Component {
        id: keyboardPageComponent
        KeyboardPage {
            onBack: navigation.pop()
        }
    }

    Component {
        id: drumsPageComponent
        DrumsPage {
            onBack: navigation.pop()
        }
    }
}
