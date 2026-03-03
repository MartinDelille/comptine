import QtQuick
import QtQuick.Controls
import Comptine

BaseDialog {
    id: renameDialog
    title: qsTr("Rename Account")

    property string originalName: ""
    okEnabled: renameField.text.trim() !== "" && renameField.text.trim() !== originalName

    onOpened: {
        originalName = AppState.data.currentAccount?.name ?? "";
        renameField.text = originalName;
        renameField.selectAll();
        renameField.forceActiveFocus();
    }

    onAccepted: {
        if (renameField.text.trim() !== "") {
            AppState.data.renameCurrentAccount(renameField.text.trim());
        }
    }

    Row {
        spacing: Theme.spacingNormal
        Label {
            text: qsTr("Account name:")
        }

        TextField {
            id: renameField
            placeholderText: qsTr("Enter account name")
            onActiveFocusChanged: {
                if (activeFocus)
                    selectAll();
            }
        }
    }
}
