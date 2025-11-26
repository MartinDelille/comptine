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

        // Custom header
        Row {
            Layout.fillWidth: true

            Repeater {
                model: [
                    {
                        title: "Date compta.",
                        width: 120
                    },
                    {
                        title: "Libellé",
                        width: 200
                    },
                    {
                        title: "Opération",
                        width: 250
                    },
                    {
                        title: "Référence",
                        width: 150
                    },
                    {
                        title: "Info complémentaires",
                        width: 200
                    },
                    {
                        title: "Type",
                        width: 130
                    },
                    {
                        title: "Catégorie",
                        width: 180
                    },
                    {
                        title: "Sous-catégorie",
                        width: 200
                    },
                    {
                        title: "Débit",
                        width: 90
                    },
                    {
                        title: "Crédit",
                        width: 90
                    },
                    {
                        title: "Date op.",
                        width: 120
                    },
                    {
                        title: "Date valeur",
                        width: 120
                    },
                    {
                        title: "Pointage",
                        width: 80
                    }
                ]

                Rectangle {
                    width: modelData.width
                    height: 35
                    color: "#e0e0e0"
                    border.width: 1
                    border.color: "#bbb"

                    Label {
                        anchors.fill: parent
                        anchors.margins: 5
                        text: modelData.title
                        font.bold: true
                        font.pixelSize: 12
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        TableView {
            id: tableView
            Layout.fillWidth: true
            Layout.fillHeight: true

            clip: true
            boundsBehavior: Flickable.StopAtBounds

            model: transactionModel

            columnWidthProvider: function (column) {
                switch (column) {
                case 0:
                    return 120;  // Date comptabilisation
                case 1:
                    return 200;  // Libellé simplifié
                case 2:
                    return 250;  // Libellé opération
                case 3:
                    return 150;  // Référence
                case 4:
                    return 200;  // Informations complémentaires
                case 5:
                    return 130;  // Type opération
                case 6:
                    return 180;  // Catégorie
                case 7:
                    return 200;  // Sous catégorie
                case 8:
                    return 90;   // Débit
                case 9:
                    return 90;   // Crédit
                case 10:
                    return 120; // Date opération
                case 11:
                    return 120; // Date de valeur
                case 12:
                    return 80;  // Pointage
                default:
                    return 100;
                }
            }

            delegate: Rectangle {
                implicitWidth: 100
                implicitHeight: 35
                border.width: 1
                border.color: "#ddd"

                required property int row
                required property int column
                required property var model

                color: {
                    if (row % 2 === 0)
                        return "#f9f9f9";
                    return "white";
                }

                Label {
                    anchors.fill: parent
                    anchors.margins: 5
                    text: getCellText()
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    font.pixelSize: 12

                    // Color for debit/credit columns
                    color: {
                        if (column === 8 && text)
                            return "#d32f2f";  // Debit in red
                        if (column === 9 && text)
                            return "#388e3c";  // Credit in green
                        return "black";
                    }

                    function getCellText() {
                        if (!model)
                            return "";

                        switch (column) {
                        case 0:
                            return model.accountingDate || "";
                        case 1:
                            return model.simplifiedLabel || "";
                        case 2:
                            return model.operationLabel || "";
                        case 3:
                            return model.reference || "";
                        case 4:
                            return model.additionalInfo || "";
                        case 5:
                            return model.operationType || "";
                        case 6:
                            return model.category || "";
                        case 7:
                            return model.subCategory || "";
                        case 8:
                            return model.debit !== 0 ? model.debit.toFixed(2) : "";
                        case 9:
                            return model.credit !== 0 ? model.credit.toFixed(2) : "";
                        case 10:
                            return model.operationDate || "";
                        case 11:
                            return model.valueDate || "";
                        case 12:
                            return model.checkStatus || "";
                        default:
                            return "";
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {}
            ScrollBar.horizontal: ScrollBar {}
        }
    }
}
