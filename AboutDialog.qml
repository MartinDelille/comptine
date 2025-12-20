import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

BaseDialog {
    id: aboutDialog
    title: qsTr("About Comptine")
    standardButtons: Dialog.Ok

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
            text: qsTr("Commit: %1").arg(AppState.appCommitHash)
            font.pointSize: 10
            opacity: 0.7
        }
    }
}
