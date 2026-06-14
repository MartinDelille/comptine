import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import commonui

BaseDialog {
    id: root

    required property var update
    title: qsTr("Update Available")
    rejectButtonText: ""
    width: 400

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
                text: qsTr("Current version: %1").arg(root.update.currentVersion())
                opacity: 0.7
            }

            Label {
                text: qsTr("Latest version: %1").arg(root.update.latestVersion)
                font.bold: true
                color: Theme.accent
            }
        }

        // Release notes section
        GroupBox {
            title: qsTr("Release Notes")
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.update.releaseNotes.length > 0

            ScrollView {
                anchors.fill: parent
                clip: true

                TextArea {
                    text: root.update.releaseNotes
                    textFormat: TextEdit.MarkdownText
                    readOnly: true
                    background: null
                }
            }
        }

        // Download button
        Button {
            text: qsTr("Download Update")
            Layout.alignment: Qt.AlignHCenter
            highlighted: true
            onClicked: {
                root.update.openDownloadPage();
            }
        }
    }
}
