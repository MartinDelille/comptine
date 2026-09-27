import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import services
import ui.common

BaseDialog {
    id: aboutDialog
    title: qsTr("About Comptine")
    rejectButtonText: ""

    ColumnLayout {
        spacing: 8

        Label {
            text: qsTr("Comptine %1").arg(AppState.appVersion)
            font.bold: true
            font.pointSize: 14
        }

        Label {
            text: qsTr("Personal Budget Management Software\n\nImport and manage your bank account data.")
        }

        Label {
            text: qsTr("Comptine is free software licensed under the GNU General Public License, version 3 or later.\n\nThe complete license text is included with the application distribution.")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Copyright © Martin Delille")
        }

        Button {
            text: qsTr("View License")
            onClicked: Qt.openUrlExternally("https://github.com/martindelille/comptine/blob/main/LICENSE.md")
        }

        Label {
            text: qsTr("Commit: %1").arg(AppState.appCommitHash)
            font.pointSize: 10
            opacity: 0.7
        }
    }
}
