import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import ui.common
import services

BaseDialog {
    id: root

    signal installRequested
    acceptCloses: false

    title: qsTr("Update Available")
    acceptButtonText: UpdateController.downloading ? qsTr("Cancel Download") : UpdateController.updateReady ? (UpdateController.installSupported ? qsTr("Install and Restart") : qsTr("Open Installer")) : qsTr("Download Update")
    width: 400

    onAcceptRequested: {
        if (UpdateController.downloading) {
            UpdateController.cancelDownload();
            root.close();
        } else if (UpdateController.updateReady) {
            root.installRequested();
            root.close();
        } else {
            UpdateController.downloadUpdate();
        }
    }

    onRejected: UpdateController.cancelDownload()

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        // Header with version info
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: qsTr("A new version of Comptine is available!")
                font.bold: true
                font.pointSize: 14
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Current version: %1").arg(UpdateController.currentVersion())
                opacity: 0.7
            }

            Label {
                text: qsTr("Latest version: %1").arg(UpdateController.latestVersion)
                font.bold: true
                color: Theme.accent
            }

            ProgressBar {
                Layout.fillWidth: true
                visible: UpdateController.downloading || UpdateController.updateReady
                value: UpdateController.downloadProgress
            }

            Label {
                Layout.fillWidth: true
                visible: UpdateController.downloading
                text: qsTr("Downloading update: %1%").arg(Math.round(UpdateController.downloadProgress * 100))
                opacity: 0.7
            }
        }

        // Release notes section
        GroupBox {
            title: qsTr("Release Notes")
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: UpdateController.releaseNotes.length > 0

            ScrollView {
                anchors.fill: parent
                clip: true

                TextArea {
                    text: UpdateController.releaseNotes
                    textFormat: TextEdit.MarkdownText
                    readOnly: true
                    background: null
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: UpdateController.updateReady && !UpdateController.installSupported
            text: qsTr("The verified installer is ready. Opening it will finish the update.")
            wrapMode: Text.WordWrap
            opacity: 0.7
        }
    }
}
