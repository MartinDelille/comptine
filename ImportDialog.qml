import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: importDialog
    title: qsTr("Import CSV")
    standardButtons: Dialog.Ok | Dialog.Cancel
    modal: true

    property string filePath: ""

    onOpened: {
        accountComboBox.currentIndex = budgetData.currentAccountIndex >= 0 ? budgetData.currentAccountIndex : 0;
    }

    onAccepted: {
        var accountName = "";
        if (accountComboBox.currentIndex >= 0 && accountComboBox.currentIndex < budgetData.accountCount) {
            accountName = budgetData.getAccount(accountComboBox.currentIndex)?.name ?? "";
        }
        budgetData.importFromCsv(filePath, accountName);
        budgetData.currentTabIndex = 0;
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingNormal

        Label {
            text: qsTr("Import into account:")
        }

        ComboBox {
            id: accountComboBox
            Layout.fillWidth: true
            Layout.preferredWidth: 250
            model: budgetData.accountCount
            displayText: currentIndex >= 0 && currentIndex < budgetData.accountCount ? budgetData.getAccount(currentIndex)?.name ?? qsTr("New account") : qsTr("New account")
            delegate: ItemDelegate {
                required property int index
                width: accountComboBox.width
                text: budgetData.getAccount(index)?.name ?? ""
                highlighted: accountComboBox.highlightedIndex === index
            }
        }
    }
}
