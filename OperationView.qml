import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Comptine

FocusScope {
    id: root
    objectName: "OperationView"

    onActiveFocusChanged: {
        if (activeFocus) {
            operationList.forceActiveFocus();
        }
    }

    function addOperation() {
        operationEditDialog.initialize(null);
    }

    function editCurrentOperation() {
        let operation = AppState.data.operationModel.operationAt(operationList.currentIndex);

        if (operation) {
            operationEditDialog.initialize(operation);
        }
    }

    RenameAccountDialog {
        id: renameDialog
    }

    OperationEditDialog {
        id: operationEditDialog
        onClosed: operationList.forceActiveFocus()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingNormal

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal

            AccountComboBox {
                Layout.preferredWidth: 200
                currentIndex: AppState.navigation.currentAccountIndex
                onActivated: function (index) {
                    AppState.navigation.currentAccountIndex = index;
                }
            }

            Button {
                text: qsTr("Rename")
                enabled: AppState.data.accountCount > 0
                onClicked: renameDialog.open()
            }

            BalanceHeader {
                Layout.fillWidth: true
                balance: AppState.data.operationModel.count > 0 ? AppState.data.operationModel.balanceAt(0) : 0
                operationCount: AppState.data.operationModel.count
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingNormal

            OperationList {
                id: operationList
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            OperationDetails {
                id: operationDetails
                Layout.preferredWidth: parent.width * 0.3
                Layout.minimumWidth: 200
                Layout.maximumWidth: 400
                Layout.fillHeight: true
                currentIndex: operationList.currentIndex
                onEditRequested: operation => {
                    operationEditDialog.initialize(operation);
                }
            }
        }
    }
}
