import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Comptine
import commonui

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
        operationEditDialog.initialize(BudgetData.currentAccount.currentOperation);
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
                currentIndex: BudgetData.currentAccountIndex
                onActivated: function (index) {
                    BudgetData.currentAccountIndex = index;
                }
            }

            Button {
                text: qsTr("Rename")
                enabled: BudgetData.accountCount > 0
                onClicked: renameDialog.open()
            }

            BalanceHeader {
                Layout.fillWidth: true
                balance: BudgetData.currentAccount?.currentBalance || 0
                operationCount: BudgetData.currentAccount?.count || 0
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingNormal

            OperationList {
                id: operationList
                account: BudgetData.currentAccount
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            OperationDetails {
                id: operationDetails
                account: BudgetData.currentAccount
                Layout.preferredWidth: parent.width * 0.3
                Layout.minimumWidth: 200
                Layout.maximumWidth: 400
                Layout.fillHeight: true
                operation: BudgetData.currentAccount?.currentOperation
                onEditRequested: operation => {
                    operationEditDialog.initialize(operation);
                }
            }
        }
    }
}
