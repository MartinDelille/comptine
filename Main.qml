import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1200
    height: 800
    visible: true
    title: qsTr("Comptine - Personal Budget Manager")

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Import CSV...")
                onTriggered: fileDialog.open()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Quit")
                onTriggered: Qt.quit()
            }
        }
        Menu {
            title: qsTr("&Help")
            Action {
                text: qsTr("&About")
                onTriggered: aboutDialog.open()
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Import CSV File")
        fileMode: FileDialog.OpenFile
        nameFilters: ["CSV files (*.csv)", "All files (*)"]
        onAccepted: {
            transactionModel.loadFromCsv(selectedFile.toString().replace("file://", ""));
            statusBar.text = qsTr("Loaded %1 transactions").arg(transactionModel.count);
        }
    }

    Dialog {
        id: aboutDialog
        title: qsTr("About Comptine")
        standardButtons: Dialog.Ok

        Label {
            text: qsTr("Comptine v0.1\n\nPersonal Budget Management Software\n\nImport and manage your bank account data.")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                text: qsTr("Import CSV")
                icon.name: "document-open"
                onClicked: fileDialog.open()
            }

            Label {
                id: statusBar
                Layout.fillWidth: true
                text: qsTr("No data loaded. Click 'Import CSV' to load transactions.")
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true

            clip: true
            boundsBehavior: Flickable.StopAtBounds

            model: transactionModel.model

            delegate: Rectangle {
                width: ListView.view.width
                height: 50
                border.width: 1
                border.color: "#ddd"

                required property int index
                required property string modelData

                Component.onCompleted: {
                    if (index === 0) {
                        console.log("Index:", index);
                        console.log("modelData type:", typeof modelData);
                        console.log("modelData value:", modelData);
                    }
                }

                color: {
                    if (index % 2 === 0)
                        return "#f9f9f9";
                    return "white";
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    Label {
                        text: "#" + (index + 1)
                        font.bold: true
                        color: "#666"
                        font.pixelSize: 14
                    }

                    Label {
                        Layout.fillWidth: true
                        text: modelData
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        font.pixelSize: 14
                        color: "#333"
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {}
        }
    }
}
