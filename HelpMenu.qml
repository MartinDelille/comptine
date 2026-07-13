import QtQuick
import QtQuick.Controls

import Comptine

Menu {
    title: qsTr("&Help")

    signal checkUpdateAction
    signal projectPageAction
    signal aboutAction

    Action {
        text: qsTr("Check for &Updates...")
        onTriggered: root.checkUpdateAction()
    }
    Action {
        text: qsTr("&Project Page")
        onTriggered: root.projectPageAction()
    }
    MenuSeparator {}
    Action {
        text: qsTr("&About Comptine")
        onTriggered: root.aboutAction()
    }
}
