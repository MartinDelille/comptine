import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1200
    height: 800
    visible: true
    title: qsTr("Comptine - La Compta qui Chante")

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

        BalanceHeader {
            balance: transactionModel.count > 0 ? transactionModel.balanceAtIndex(0) : 0
            transactionCount: transactionModel.count
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            TransactionList {
                id: transactionList
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: transactionModel.model
            }

            TransactionDetails {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                transaction: transactionList.currentIndex >= 0 ? transactionModel.getTransaction(transactionList.currentIndex) : null
                balance: transactionList.currentIndex >= 0 ? transactionModel.balanceAtIndex(transactionList.currentIndex) : 0
            }
        }
    }
}
